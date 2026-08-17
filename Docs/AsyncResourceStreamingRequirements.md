# 비동기 리소스 스트리밍 및 엔진 진단 요구사항

## 1. 목적

DungeonSync의 런타임 텍스처 로딩을 동기 방식과 비동기 방식으로 비교하고, 프레임 히치·메모리·리소스 상태를 인게임 진단 오버레이에서 관찰할 수 있도록 한다.

이 기능은 다음 채용 역량을 하나의 검증 가능한 흐름으로 보여주는 것을 목표로 한다.

- DX11 엔진 기능 개선
- 렌더링 및 메모리 최적화
- Windows 병렬 프로그래밍
- 파일 입출력과 자원 수명 관리
- 클라이언트 안정성 강화
- 공통 개발 도구 제작

## 2. 현재 구조와 개선 대상

현재 `WicTextureLoader::LoadFromFile`은 다음 작업을 한 함수에서 수행한다.

1. 파일 열기
2. WIC 디코더 생성
3. RGBA8 픽셀 변환
4. CPU 픽셀 버퍼 할당
5. `ID3D11Texture2D` 생성
6. `ID3D11ShaderResourceView` 생성

따라서 런타임에 호출하면 파일 I/O와 디코딩이 메인 스레드를 점유하여 프레임 히치를 만들 수 있다. 또한 `D3D11Renderer`가 로딩과 텍스처 소유까지 담당하여 책임 범위가 넓다.

개선 후에는 CPU 작업과 GPU 작업을 분리한다.

```text
Worker thread
  File I/O → WIC decode → RGBA8 DecodedImage
                         ↓ completed queue
Main thread
  Texture upload → SRV creation → cache publish → scene binding
```

## 3. 범위

### 포함

- PNG 텍스처의 동기 및 비동기 로딩
- 작업 스레드 1개와 요청/완료 큐
- 텍스처 상태 및 경로 기반 캐시
- 메인 스레드 GPU 업로드
- 방별 지면 텍스처 프리로드와 교체
- 필수 리소스 미준비 시 로딩 오버레이
- 동기/비동기 히치 비교 테스트
- CPU/GPU 메모리 추정치와 큐 상태 수집
- Direct2D/DirectWrite 기반 진단 텍스트 오버레이
- 오류 및 종료 경로 검증

### 제외

- 범용 에셋 번들 또는 패치 시스템
- 압축 아카이브와 암호화
- GPU 메모리 예산 초과 시 LRU 자동 퇴출
- 다중 작업 스레드 풀
- 셰이더 및 메시 스트리밍
- DirectStorage

## 4. 설계 원칙

1. 작업 스레드는 D3D11 렌더 상태와 장면 객체를 수정하지 않는다.
2. GPU 리소스 생성과 캐시 공개는 메인 스레드의 명시적인 처리 지점에서 수행한다.
3. 큐를 통해 전달된 데이터는 단일 소유권을 가지며, 업로드 완료 또는 실패 시 CPU 픽셀 메모리를 즉시 해제한다.
4. 동일 경로의 중복 요청은 하나의 로딩 작업으로 합친다.
5. 종료 시 새 요청을 차단하고 작업 스레드를 깨운 뒤 `join`하여 모든 자원 소멸 순서를 보장한다.
6. 진단 도구의 렌더 비용은 장면 CPU/GPU 측정 구간과 분리한다.
7. 기존 시작 시 동기 로딩 경로는 첫 단계에서 유지하여 회귀 범위를 제한한다.

## 5. 데이터 모델

### TextureLoadState

```cpp
enum class TextureLoadState
{
    Unloaded,
    Queued,
    Loading,
    ReadyForUpload,
    Ready,
    Failed
};
```

허용 상태 전이는 다음과 같다.

```text
Unloaded → Queued → Loading → ReadyForUpload → Ready
                         └────────────────────→ Failed
Unloaded → Loading → Ready                         // 동기 경로
```

완료된 텍스처를 다시 요청하면 새 작업을 만들지 않고 기존 `Ready` 항목을 반환한다.

### DecodedImage

CPU 측 디코딩 결과이며 작업 스레드에서 완료 큐로 이동한다.

```text
requestId
normalizedPath
width
height
rowPitch
pixelFormat = RGBA8
pixels: vector<uint8_t>
decodeMilliseconds
errorCode
```

픽셀 바이트 수는 오버플로를 검사한 `rowPitch × height`로 계산한다.

### TextureResource

메인 스레드가 소유한다.

```text
state
path
shaderResourceView
width / height
estimatedGpuBytes
lastLoadMilliseconds
error
```

## 6. 클래스 책임

### WicImageDecoder

- WIC를 사용해 파일을 RGBA8 `DecodedImage`로 변환한다.
- D3D11 타입을 알지 못한다.
- 작업 스레드에서 별도의 COM MTA 초기화 후 사용한다.
- 디코드 실패 원인을 결과에 저장한다.

### AsyncTextureLoader

- 작업 스레드의 생성과 종료를 관리한다.
- 요청 큐와 완료 큐를 소유한다.
- `mutex`, `condition_variable`, 종료 플래그를 사용한다.
- 경로별 중복 요청을 막는다.
- 큐 통계와 디코드 중인 작업 수를 제공한다.
- 복사와 이동을 금지한다.

### TextureResourceManager

- 텍스처 캐시와 `TextureResource` 수명을 관리한다.
- 동기 로딩과 비동기 요청 API를 제공한다.
- 완료된 `DecodedImage`를 프레임당 제한된 수만 GPU에 업로드한다.
- 준비된 SRV를 Renderer에 제공한다.
- 메모리 및 상태 통계를 제공한다.

### D3D11Renderer

- 텍스처 파일을 직접 디코딩하지 않는다.
- 전달받은 SRV를 렌더 패스에 바인딩한다.
- 인스턴스 버퍼 메모리와 재할당 횟수를 통계로 제공한다.
- 장면 측정 종료 후 진단 오버레이를 그린다.

### EngineDiagnostics

- 렌더링, 스트리밍, 메모리 측정값을 하나의 읽기 전용 스냅샷으로 구성한다.
- Renderer나 로더의 소유권을 갖지 않는다.
- 오버레이와 로그가 동일한 스냅샷을 사용하게 한다.

## 7. 스레드 및 COM 요구사항

- 작업 스레드 진입 직후 `CoInitializeEx(nullptr, COINIT_MULTITHREADED)`를 호출한다.
- 성공 또는 `RPC_E_CHANGED_MODE` 등 결과를 검사하고 실패 상태를 기록한다.
- WIC Factory와 디코더는 작업 스레드 내부에서 생성·사용·해제한다.
- 작업 스레드 종료 전에 `CoUninitialize`를 올바르게 호출한다.
- 조건 변수의 대기 조건은 `stopRequested || !requestQueue.empty()`이다.
- 소멸자는 종료 요청, `notify_all`, `join` 순서로 완료한다.
- 종료 요청 이후 들어오는 로딩 요청은 거부한다.
- 완료 큐로 이동한 `DecodedImage`는 메인 스레드만 소비한다.

## 8. 큐 및 프레임 안정성

- 최대 요청 큐: 32개
- 최대 완료 큐: 8개 또는 메모리 상한으로 제한
- 프레임당 GPU 업로드: 기본 1개
- 큐가 가득 차면 요청 실패를 반환하고 통계를 증가시킨다.
- GPU 업로드 실패 시 캐시 상태를 `Failed`로 바꾸고 CPU 픽셀 버퍼를 해제한다.
- 한 프레임에 모든 완료 리소스를 업로드하여 새로운 히치를 만들지 않는다.

## 9. 방 전환 및 로딩 화면

초기 필수 리소스는 기존처럼 동기 로딩하여 첫 화면의 실패 처리를 단순하게 유지한다.

런타임 흐름은 다음과 같다.

1. 현재 방 전투 시작 시 다음 방 지면 텍스처를 비동기 프리로드한다.
2. 다음 방으로 전환되면 해당 텍스처 상태를 확인한다.
3. `Ready`이면 즉시 텍스처를 교체하고 플레이를 계속한다.
4. 준비되지 않았으면 DemoScene 업데이트와 입력 적용을 일시 중지한다.
5. 렌더 루프와 네트워크 수신, Windows 메시지 처리는 계속한다.
6. 반투명 또는 검은 로딩 오버레이와 진행 상태를 표시한다.
7. 필수 리소스가 `Ready`가 되면 다음 프레임부터 게임을 재개한다.
8. `Failed`이면 오류 메시지와 재시도 또는 안전 종료 선택지를 제공한다.

프리로드가 정상적으로 완료되면 로딩 화면이 보이지 않을 수 있다. 진단 모드에서는 캐시를 우회하여 로딩 화면과 상태 전이를 재현할 수 있어야 한다.

## 10. 동기/비동기 히치 비교

### 입력

| 키 | 동작 |
|---|---|
| `9` | 동일한 리소스 세트를 메인 스레드에서 동기 로딩 |
| `0` | 동일한 리소스 세트를 작업 스레드에서 비동기 디코딩 |
| `-` | 엔진 진단 오버레이 표시/숨김 |
| `=` | 기본 지면 텍스처로 복귀 |

숫자키 7/8 자동 렌더링 벤치마크와 충돌하지 않도록 해당 세션 실행 중에는 9/0 요청을 거부한다.

### 공정한 비교 조건

- 두 모드는 동일한 파일 목록과 동일한 WIC RGBA8 변환을 사용한다.
- 캐시된 `Ready` 리소스는 비교 대상에서 제외한다.
- OS 파일 캐시는 완전히 통제할 수 없으므로 파일 I/O와 디코딩 시간을 구분하여 기록한다.
- 테스트는 Release 빌드에서 각각 여러 번 수행한다.
- 인위적인 `sleep` 결과를 최종 개선 수치로 사용하지 않는다.
- GPU 업로드 비용은 두 모드 모두 메인 스레드에서 별도로 기록한다.

### 측정 항목

- Request-to-Ready 시간
- WIC Decode 시간
- GPU Upload 시간
- Frame Avg/P95/P99/Max
- 16.67ms 및 33.33ms 초과 프레임 수
- 비교 구간의 샘플 수
- 테스트 전후 CPU decoded bytes와 GPU texture bytes

## 11. 메모리 통계

### 추적 항목

```text
Texture resource count
Ready / Loading / Failed count
Estimated GPU texture bytes
Decoded CPU bytes awaiting upload
Peak decoded CPU bytes
Instance buffer bytes
Instance buffer resize count
Effect pool active / capacity
Request queue count
Completion queue count
Queue rejection count
Upload failure count
```

RGBA8, Mip 1 텍스처의 GPU 메모리 추정값은 다음과 같다.

```text
width × height × 4 bytes
```

이는 드라이버 내부 정렬과 복사본을 포함한 실제 전체 VRAM 사용량이 아니라 프로젝트가 생성한 텍스처의 논리적 추정치임을 오버레이와 문서에 명시한다.

인스턴스 버퍼 메모리는 다음으로 계산한다.

```text
instanceBufferCapacity × sizeof(InstanceData)
```

## 12. 인게임 엔진 진단 오버레이

Direct2D/DirectWrite를 DXGI 백버퍼와 연동하여 텍스트를 렌더링한다. 이를 위해 D3D11 장치 생성 시 BGRA 지원 플래그를 사용하고 CMake에 필요한 Windows 라이브러리를 연결한다.

오버레이는 장면 GPU timestamp 종료 후, Present 전에 렌더링한다. 따라서 장면 GPU 수치에 진단 도구 자체의 그리기 비용을 섞지 않는다. 전체 Frame과 Present 시간에는 실제 사용자 프레임의 일부로 반영된다.

표시 항목:

```text
PERFORMANCE
FPS / Frame Avg / P95 / P99 / Max
CPU Submit / GPU / Present
Frames >16.67ms / >33.33ms

RENDERING
Draw Calls
Submitted / Rendered / Dropped Instances
Instance Capacity / Buffer Bytes / Resize Count

RESOURCE STREAMING
Mode: Idle / Sync / Async
Queued / Decoding / Awaiting Upload
Ready / Failed
Decoded CPU Bytes / Peak
Estimated Texture GPU Bytes
Last Decode / Upload / Request-to-Ready
```

### 오버레이 성능 요구사항

- 기본적으로 4~10Hz로 문자열을 갱신하고 매 프레임 새 문자열 조합을 최소화한다.
- 표시/숨김이 가능하다.
- 자동 벤치마크 중에는 기본적으로 숨기거나 측정 구간 밖에서 그린다.
- 렌더 실패가 게임 전체 실패로 이어지지 않도록 오버레이만 비활성화하고 로그를 남긴다.

## 13. 오류 처리

- 존재하지 않는 파일
- 지원하지 않는 이미지 또는 손상된 PNG
- 크기 계산 오버플로
- 요청 큐 포화
- 종료 중 새 요청
- COM/WIC 초기화 실패
- D3D11 Texture 또는 SRV 생성 실패
- 중복 요청

각 실패는 상태, 경로, 오류 코드와 함께 기록한다. 실패한 항목은 `Ready` 캐시에 들어가지 않는다.

## 14. 검증 시나리오

### 기능

- 기존 세 텍스처가 정상 출력된다.
- 비동기 요청 후 메인 루프가 계속 렌더링된다.
- 다음 방 프리로드와 지면 교체가 정상 동작한다.
- 미준비 상태에서는 로딩 오버레이가 표시되고 입력이 적용되지 않는다.
- 준비 완료 후 자동으로 게임이 재개된다.

### 동시성과 종료

- 요청 큐가 비어 있는 상태에서 종료
- 디코딩 중 종료
- 완료 큐에 픽셀 데이터가 남은 상태에서 종료
- 같은 경로를 반복 요청
- 실패한 파일과 정상 파일을 연속 요청
- 종료 후 Worker thread가 남지 않음

### 측정

- 9 동기 테스트에서 메인 스레드 히치가 Frame Max에 기록됨
- 0 비동기 테스트에서 렌더 루프가 유지됨
- CPU decode, GPU upload, Request-to-Ready 값이 각각 기록됨
- 오버레이 표시 여부가 장면 GPU 측정값을 오염시키지 않음
- 메모리 카운터가 로딩 전후 및 실패 후 예상대로 복원됨
- Windows Working Set, Peak Working Set, Private Bytes를 주기적으로 수집함
- 텍스처와 인스턴스 버퍼의 추적 GPU 메모리를 프로세스 메모리와 구분해 표시함

## 15. 완료 조건

- Debug와 Release가 경고 없이 빌드된다.
- 기존 게임플레이, 네트워크, 렌더링 벤치마크가 회귀하지 않는다.
- 동기/비동기 비교를 동일 조건에서 재현할 수 있다.
- 비동기 로딩 중 Windows 메시지, 렌더링, 네트워크 수신이 계속된다.
- 종료 시 작업 스레드와 CPU 디코딩 버퍼가 남지 않는다.
- 오버레이가 렌더·리소스·메모리 수치를 실제 엔진 상태에서 표시한다.
- Release 측정 결과와 한계를 README에 기록한다.

## 16. 구현 순서

1. 기존 WIC 로더를 `Decode`와 `Upload`로 분리하되 동기 로딩 동작을 유지한다.
2. `AsyncTextureLoader`와 안전한 종료를 구현한다.
3. `TextureResourceManager` 캐시와 메인 스레드 업로드를 구현한다.
4. 9/0 비교 시나리오와 스트리밍 통계를 구현한다.
5. 방별 프리로드와 로딩 상태 게이트를 연결한다.
6. 인스턴스 버퍼·이펙트 풀·텍스처 메모리 통계를 통합한다.
7. Direct2D/DirectWrite 진단 오버레이를 구현한다.
8. Release QA, 반복 측정, README 및 그래프 갱신을 수행한다.

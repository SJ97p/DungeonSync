"use strict";

const evidence = {
  baseline: { src: "Assets/evidence/diagnostics-baseline.png", caption: "기본 장면 · Draw 3 · 인스턴스 버퍼 96 KiB" },
  instanced: { src: "Assets/evidence/diagnostics-10000-instanced.png", caption: "10,000 인스턴스 · Draw 3 · 추적 GPU 1.50 MiB" },
  streamed: { src: "Assets/evidence/diagnostics-streamed-textures.png", caption: "비동기 텍스처 7/7 · 추적 GPU 24.50 MiB · 최대 디코드 8 MiB" },
  retained: { src: "Assets/evidence/diagnostics-cache-retained.png", caption: "기본 지면 복귀 후 캐시와 최대 사용 버퍼 유지" },
  benchmark: { src: "Docs/Images/rendering_benchmark.svg", caption: "Release 자동 벤치마크 · Instanced vs Per-instance" }
};

const code = (path, focus = "", methods = []) => ({ path, focus, methods });

const nodes = {
  overview: {
    type: "system",
    title: "DungeonSync 엔진 연구 개요",
    summary: "Unity에서 경험한 프레임 지연과 리소스 문제를 DX11의 렌더링·스트리밍·진단·네트워크 경계까지 내려가 관찰한 작은 실험 환경입니다.",
    intent: "화려한 콘텐츠 제작보다 플레이어가 체감하는 히치와 지연을 어디서 찾아야 하는지 직접 확인하고, 안정성·관찰 가능성·렌더링 효율을 설명 가능한 증거로 남기는 것이 목표입니다.",
    issue: "단순 데모는 화면이 동작한다는 사실만 보여줄 뿐, 병목의 위치나 개선 효과를 증명하기 어렵습니다. CPU 제출, GPU 실행, Present 대기, 리소스 로딩과 메모리를 분리해서 관찰할 필요가 있었습니다.",
    final: "D3D11 2.5D 렌더러, 인스턴싱 비교, 비동기 텍스처 파이프라인, 엔진 진단 오버레이, 공간 분할 전투, TCP 상태 동기화를 하나의 C++20 실행 환경에 결합했습니다. Debug/Release 양쪽 빌드와 재현 가능한 측정으로 검증했습니다.",
    next: "전체 VRAM 예산은 DXGI QueryVideoMemoryInfo로 확장할 수 있고, 네트워크는 다중 클라이언트 IO 모델과 패킷 시뮬레이션이 다음 실험 대상입니다. 현재 결과는 구현된 범위와 한계를 명시해 과장하지 않습니다.",
    graph: `flowchart LR
      APP[Application] --> SCENE[DemoScene / 전투]
      APP --> STREAM[비동기 스트리밍]
      APP --> NET[TCP 상태 동기화]
      SCENE --> RENDER[DX11 렌더러]
      STREAM --> RENDER
      RENDER --> GPU[GPU / Present]
      RENDER --> PROF[성능 측정]
      STREAM --> MEM[메모리 지표]
      PROF --> HUD[진단 오버레이]
      MEM --> HUD`,
    links: { APP: "application", SCENE: "demoscene", STREAM: "streaming", NET: "network", RENDER: "renderer", GPU: "renderer", PROF: "diagnostics", HUD: "diagnosticsoverlay", MEM: "memory" },
    scripts: ["application", "renderer", "streaming", "diagnostics", "network"],
    evidence: ["baseline", "instanced", "streamed", "benchmark"],
    report: "overview"
  },

  renderer: {
    type: "system",
    title: "DX11 2.5D 렌더링 파이프라인",
    summary: "직교 카메라와 X-Z 지상 좌표계 위에 배경·지면·스프라이트 패스를 분리한 DirectX 11 렌더러입니다.",
    intent: "DX11의 디바이스, 스왑체인, 백버퍼, 깊이 버퍼와 GPU 파이프라인의 책임을 직접 다루면서 던전형 2.5D 장면에 필요한 렌더 순서를 구성했습니다.",
    issue: "불투명 지면과 알파 스프라이트는 깊이 테스트·쓰기와 블렌딩 규칙이 다릅니다. 모든 객체를 개별 호출하면 장면 규모가 커질수록 CPU/GPU 제출 비용도 증가합니다.",
    final: "배경→지면→스프라이트 순서로 패스를 분리하고, 스프라이트는 공통 메시와 텍스처를 재사용해 인스턴스 데이터만 GPU에 제출합니다. 장면 GPU timestamp query는 오버레이 패스 전에 종료합니다.",
    next: "텍스처 배열, 재질별 배칭, 가시성 컬링, 동적 해상도와 실제 GPU 메모리 예산 추적은 현재 범위 밖입니다.",
    graph: `flowchart LR
      FRAME[프레임 시작] --> BG[배경 패스]
      BG --> GROUND[지면 패스]
      GROUND --> SPRITE[스프라이트 패스]
      SPRITE --> QUERY[GPU 측정 종료]
      QUERY --> OVERLAY[D2D 진단 오버레이]
      OVERLAY --> PRESENT[SwapChain Present]`,
    links: { FRAME: "frametimeprofiler", BG: "d3d11renderer", GROUND: "d3d11renderer", SPRITE: "instancing", QUERY: "diagnostics", OVERLAY: "diagnosticsoverlay", PRESENT: "d3d11renderer" },
    scripts: ["d3d11renderer", "diagnosticsoverlay", "demoscene"],
    evidence: ["baseline", "instanced"]
  },

  instancing: {
    type: "system",
    title: "인스턴싱 및 제출 방식 비교",
    summary: "100~10,000개 스프라이트를 동일 조건에서 Instanced와 Per-instance 제출 방식으로 비교합니다.",
    intent: "드로우콜 감소가 실제 CPU 제출과 GPU 시간에 어떤 영향을 주는지 체감이 아니라 동일 시나리오로 측정하고자 했습니다.",
    issue: "FPS만 보면 240Hz Present 대기에 가려 두 제출 방식의 차이가 작아 보입니다. CPU Submit, GPU timestamp, Present를 분리해야 원인을 볼 수 있습니다.",
    final: "10,000개에서 Draw Call을 10,002회에서 3회로 줄였습니다. Release 실행 검증에서 GPU P99는 약 6.14ms에서 0.05ms 수준으로 감소했습니다. 동적 인스턴스 버퍼는 2배 성장 전략으로 16,384 capacity를 확보합니다.",
    next: "드라이버·GPU별 반복 측정과 bootstrap confidence interval을 추가하면 결과의 통계적 신뢰도를 높일 수 있습니다.",
    graph: `flowchart LR
      ITEMS[렌더 항목<br/>10,000개] --> A{제출 방식}
      A -->|Per-instance| MANY[스프라이트 Draw<br/>10,000회]
      A -->|Instanced| ONE[스프라이트 Batch<br/>1회]
      MANY --> METRIC[CPU / GPU / Present 지표]
      ONE --> METRIC
      METRIC --> CSV[CSV 측정 결과]`,
    links: { ITEMS: "renderingstressscene", MANY: "d3d11renderer", ONE: "d3d11renderer", METRIC: "frametimeprofiler", CSV: "benchmarksession" },
    scripts: ["renderingstressscene", "d3d11renderer", "benchmarksession", "frametimeprofiler"],
    evidence: ["benchmark", "instanced"],
    report: "rendering"
  },

  streaming: {
    type: "system",
    title: "비동기 텍스처 스트리밍",
    summary: "CPU 디코드와 GPU 업로드를 분리하고 제한 큐·캐시·안전한 종료를 갖춘 텍스처 스트리밍 실험입니다.",
    intent: "온라인 게임에서 순간적인 리소스 로딩이 한 프레임을 막아 플레이 감각을 훼손하는 히칭을 재현하고 줄이는 것이 연구 질문입니다.",
    issue: "WIC 디코드와 GPU 리소스 생성을 메인 스레드에서 연속 수행하면 동일한 7장 로딩에서 한 프레임이 56ms 이상 멈췄습니다. 반대로 D3D context를 무분별하게 작업 스레드에서 다루면 동시성과 수명 관리가 복잡해집니다.",
    final: "Worker는 COM MTA와 WIC factory를 소유해 RGBA8 픽셀까지만 생성합니다. 완료 데이터는 bounded queue로 전달하고 메인 스레드가 프레임당 1장만 GPU에 업로드합니다. Release에서 Frame Max 57.416ms를 6.798ms로 낮추고 16.67ms 초과를 1회에서 0회로 줄였습니다.",
    next: "현재 한 개 Worker와 FIFO 정책입니다. 우선순위, 취소 토큰, 방 단위 dependency group, GPU upload budget을 추가할 수 있습니다.",
    graph: `flowchart LR
      REQUEST[메인 스레드 요청] --> RQ[제한 요청 큐]
      RQ --> WORKER[Worker + WIC 디코드]
      WORKER --> CQ[완료 RGBA 큐]
      CQ --> UPLOAD[프레임당 1회 업로드]
      UPLOAD --> CACHE[텍스처 캐시]
      CACHE --> DRAW[지면 텍스처 교체]`,
    links: { REQUEST: "textureresourcemanager", RQ: "asynctextureloader", WORKER: "asynctextureloader", CQ: "textureresourcemanager", UPLOAD: "textureresourcemanager", CACHE: "textureresourcemanager", DRAW: "d3d11renderer" },
    scripts: ["asynctextureloader", "textureresourcemanager", "wictextureloader", "d3d11renderer"],
    evidence: ["streamed", "retained"],
    report: "streaming"
  },

  diagnostics: {
    type: "system",
    title: "인게임 엔진 진단 도구",
    summary: "프레임·제출·GPU·Present·메모리·스트리밍 상태를 런타임에서 한 화면으로 관찰합니다.",
    intent: "라이브 이슈에 대응하려면 평균 FPS가 아니라 P99, Max, 히치 횟수와 병목 구간을 즉시 확인할 수 있어야 합니다.",
    issue: "Present 대기가 포함된 Frame time만으로는 CPU 제출과 GPU 실행 비용을 구분할 수 없습니다. 진단 UI 자체가 측정 구간을 오염시키는 문제도 피해야 합니다.",
    final: "최근 600샘플의 Avg/P95/P99/Max와 16.67/33.33ms 초과 횟수를 유지합니다. D3D11 비동기 timestamp query로 GPU를 측정하고 D2D 오버레이는 장면 query 종료 뒤에 그립니다.",
    next: "오버레이 자체 비용의 별도 profiler, 지표 히스토그램, 프레임 캡처 bookmark와 원격 telemetry 전송이 확장 방향입니다.",
    graph: `flowchart TB
      FRAME[프레임 시간] --> FP[FrameTimeProfiler]
      SUBMIT[CPU 제출] --> FP
      GPUQ[D3D11 타임스탬프] --> FP
      PRESENT[Present 대기] --> FP
      MEMORY[메모리 측정] --> HUD[DiagnosticsOverlay]
      FP --> HUD`,
    links: { FRAME: "frametimeprofiler", SUBMIT: "frametimeprofiler", FP: "frametimeprofiler", GPUQ: "d3d11renderer", PRESENT: "d3d11renderer", MEMORY: "processmemorysampler", HUD: "diagnosticsoverlay" },
    scripts: ["frametimeprofiler", "diagnosticsoverlay", "processmemorysampler", "d3d11renderer"],
    evidence: ["baseline", "instanced", "streamed"]
  },

  memory: {
    type: "system",
    title: "메모리 사용량 관측",
    summary: "OS 프로세스 메모리와 엔진이 추적하는 리소스 메모리를 의도적으로 구분합니다.",
    intent: "메모리 증가를 곧바로 누수로 단정하지 않고 실제 RAM residency, private commit, 캐시, GPU 리소스 high-water mark를 구분하는 것이 목표입니다.",
    issue: "Working Set은 OS가 trimming할 수 있고 Private Bytes는 공유 불가능한 commit을 나타냅니다. 엔진 추정 GPU 값은 드라이버 전체 VRAM과 동일하지 않습니다.",
    final: "GetProcessMemoryInfo로 Working/Peak/Private를 수집하고 텍스처 RGBA8 크기와 instance capacity를 별도 합산합니다. 7장 로딩에서 tracked texture 23MiB, peak decoded 8MiB, 10,000 인스턴스에서 buffer 1.5MiB를 확인했습니다.",
    next: "현재 시작 시 직접 로딩한 기본 텍스처와 동기 비교 세트는 tracked GPU 합계에 포함되지 않습니다. DXGI adapter budget과 모든 resource registry 통합이 필요합니다.",
    graph: `flowchart TB
      OS[Windows 프로세스 카운터] --> WS[Working Set / Private Bytes]
      CACHE[텍스처 캐시] --> TEX[추정 텍스처 GPU]
      BUFFER[인스턴스 용량] --> IB[인스턴스 버퍼 크기]
      DECODE[디코드 픽셀] --> CPU[대기 / 최대 디코드]
      TEX --> TRACKED[추적 GPU 합계]
      IB --> TRACKED`,
    links: { OS: "processmemorysampler", WS: "processmemorysampler", CACHE: "textureresourcemanager", TEX: "textureresourcemanager", BUFFER: "d3d11renderer", IB: "d3d11renderer", DECODE: "asynctextureloader", CPU: "asynctextureloader", TRACKED: "processmemorysampler" },
    scripts: ["processmemorysampler", "textureresourcemanager", "d3d11renderer"],
    evidence: ["baseline", "instanced", "streamed", "retained"]
  },

  combat: {
    type: "system",
    title: "공간 분할 기반 전투 판정",
    summary: "Uniform Spatial Grid의 broad phase와 거리·부채꼴 narrow phase로 공격 후보를 줄입니다.",
    intent: "2.5D 평면에서 비교적 균일하게 배치된 몬스터와 고정 반경 공격에 맞춰, 모든 객체를 검사하는 대신 판정을 놓치지 않는 저비용 후보 수집과 정확한 최종 판정을 분리합니다.",
    issue: "셀을 너무 크게 하면 후보가 많아지고 너무 작게 하면 셀 순회 비용이 늘어납니다. broad phase 결과만으로 피격을 확정하면 원 밖의 객체가 맞습니다.",
    final: "월드 좌표를 cell size로 나눠 floor한 정수 키로 저장합니다. 공격 원을 감싸는 AABB의 셀만 수집한 뒤 거리 제곱 또는 방향 dot 조건으로 최종 판정합니다.",
    next: "동적 객체 갱신 비용, 비균일 분포, 대규모 맵에서는 loose quadtree나 BVH와 비교할 수 있습니다.",
    graph: `flowchart LR
      WORLD[몬스터 위치] --> GRID[Uniform Spatial Grid]
      ATTACK[공격 범위] --> CELLS[AABB 셀 범위]
      GRID --> CELLS
      CELLS --> BROAD[Broad Phase 후보]
      BROAD --> NARROW[거리² / 부채꼴 검사]
      NARROW --> HIT[최종 피격 대상]`,
    links: { WORLD: "demoscene", GRID: "spatialgrid", ATTACK: "demoscene", CELLS: "spatialgrid", BROAD: "combatsystem", NARROW: "combatsystem", HIT: "demoscene" },
    scripts: ["spatialgrid", "combatsystem", "demoscene"],
    evidence: []
  },

  network: {
    type: "system",
    title: "TCP 상태 동기화",
    summary: "RAII 소켓, 고정 패킷, 수신 스레드와 서버 승인 위치 보정으로 클라이언트·서버 흐름을 구성합니다.",
    intent: "렌더링 데모를 넘어 온라인 클라이언트가 입력 예측과 authoritative 상태를 어떻게 연결하고 안전하게 종료하는지 구현합니다.",
    issue: "TCP는 순서와 신뢰성을 보장하지만 메시지 경계를 보장하지 않습니다. 수신 스레드와 메인 스레드가 동일 상태를 다루면 데이터 경쟁과 종료 교착 위험이 있습니다.",
    final: "20Hz 위치 패킷과 sequence를 전송하고 서버가 이동량을 검증해 snapshot을 회신합니다. Receiver는 최신 상태만 mutex로 게시하고 메인 스레드가 소비합니다. Stop flag, shutdown, join 순서로 종료합니다.",
    next: "현재 단일 클라이언트 고정 크기 실험입니다. 수신 누적 버퍼, 다중 세션, 비정상 패킷 fuzzing과 지연·손실 시뮬레이션이 다음 단계입니다.",
    graph: `sequenceDiagram
      participant C as 클라이언트 메인
      participant S as TCP 서버
      participant R as 수신 스레드
      C->>S: PlayerMove(sequence, x, y)
      S->>S: 이동량 검증
      S->>R: PlayerStateSnapshot
      R->>R: mutex로 최신 상태 게시
      C->>R: TryConsumeLatest
      C->>C: Snap 또는 보간`,
    links: { "클라이언트 메인": "application", "TCP 서버": "tcplistener", "수신 스레드": "serverstatereceiver" },
    scripts: ["tcpclient", "serverstatereceiver", "packet", "tcplistener", "application"],
    evidence: []
  },

  application: { type: "class", title: "Application", summary: "초기화 순서와 메인 루프를 조정하는 composition root입니다.", intent: "하위 시스템의 소유권과 수명을 한곳에서 명확히 합니다.", issue: "입력·진단 문자열 조립까지 포함되어 현재 파일 크기가 커진 상태입니다.", final: "Winsock→TCP→Window→Renderer→ResourceManager 순으로 시작하고 역순으로 종료합니다.", next: "InputController와 DiagnosticsPresenter 분리가 다음 리팩터링 후보입니다.", graph: `classDiagram
      class Application {
        +Run() int
      }
      Application *-- D3D11Renderer
      Application *-- TextureResourceManager
      Application *-- ServerStateReceiver
      Application *-- DemoScene`, links: { D3D11Renderer: "d3d11renderer", TextureResourceManager: "textureresourcemanager", ServerStateReceiver: "serverstatereceiver", DemoScene: "demoscene" }, scripts: [], source: code("Client/Application.cpp", "int Application::Run", ["Application::Run"]) },
  d3d11renderer: { type: "class", title: "D3D11Renderer", summary: "DX11 자원과 렌더 패스를 소유합니다.", intent: "장면 제출과 GPU 자원 수명을 한 컴포넌트가 관리합니다.", issue: "패스가 늘면 단일 클래스가 커질 수 있습니다.", final: "배경·지면·스프라이트, GPU query, Present 구간을 명확한 순서로 실행합니다.", next: "RenderGraph 또는 pass 객체 분리를 고려할 수 있습니다.", graph: `classDiagram
      class D3D11Renderer {
        +Initialize() bool
        +Render() void
        +UploadDecodedTexture() bool
      }
      D3D11Renderer *-- DiagnosticsOverlay
      D3D11Renderer *-- WicTextureLoader`, links: { DiagnosticsOverlay: "diagnosticsoverlay", WicTextureLoader: "wictextureloader" }, scripts: [], source: code("Client/Rendering/D3D11Renderer.cpp", "void D3D11Renderer::Render", ["D3D11Renderer::Render", "D3D11Renderer::UploadDecodedTexture"]) },
  asynctextureloader: { type: "class", title: "AsyncTextureLoader", summary: "bounded producer-consumer queue와 worker lifetime을 관리합니다.", intent: "CPU 디코드를 메인 루프에서 분리합니다.", issue: "완료 큐가 가득 찰 때 worker가 안전하게 대기해야 합니다.", final: "condition_variable predicate에 stop과 queue 상태를 포함하고 Stop에서 notify_all 후 join합니다.", next: "우선순위·취소·다중 worker 정책이 확장점입니다.", graph: `classDiagram
      class AsyncTextureLoader {
        +Start() bool
        +Request(path) optional~id~
        +TryPopCompleted() bool
        +Stop() void
      }
      AsyncTextureLoader *-- WicTextureLoader`, links: { WicTextureLoader: "wictextureloader" }, scripts: [], source: code("Client/Rendering/AsyncTextureLoader.cpp", "bool AsyncTextureLoader::Start", ["AsyncTextureLoader::Start", "AsyncTextureLoader::Request", "AsyncTextureLoader::WorkerMain", "AsyncTextureLoader::Stop"]) },
  textureresourcemanager: { type: "class", title: "TextureResourceManager", summary: "요청 상태, 캐시, 메인 스레드 업로드와 메모리 통계를 관리합니다.", intent: "비동기 작업과 렌더러 사이의 정책 계층입니다.", issue: "GPU API 호출 위치와 resource state 전이가 명확해야 합니다.", final: "Unloaded→Queued→ReadyForUpload→Ready/Failed 상태와 path cache를 유지합니다.", next: "Eviction policy와 resource group dependency가 필요합니다.", graph: `flowchart LR
      REQUEST[RequestAsync] --> MANAGER[TextureResourceManager]
      MANAGER --> LOADER[AsyncTextureLoader]
      LOADER --> WIC[WicTextureLoader]
      LOADER --> READY[ReadyForUpload]
      READY --> RENDERER[D3D11Renderer]
      MANAGER --> CACHE[Path Cache]`, links: { LOADER: "asynctextureloader", WIC: "wictextureloader", RENDERER: "d3d11renderer" }, scripts: [], source: code("Client/Rendering/TextureResourceManager.cpp", "bool TextureResourceManager::RequestAsync", ["TextureResourceManager::RequestAsync", "TextureResourceManager::Update", "TextureResourceManager::Statistics"]) },
  wictextureloader: { type: "class", title: "WicTextureLoader", summary: "WIC 파일 디코드와 D3D11 texture creation을 분리합니다.", intent: "CPU 작업과 GPU 작업의 경계를 API에 반영합니다.", issue: "COM apartment와 WIC factory는 worker 소유권이 필요합니다.", final: "DecodeFromFile은 RGBA8 pixels, CreateTextureFromDecodedImage는 SRV를 생성합니다.", next: "mipmap 생성과 압축 포맷 지원이 남아 있습니다.", graph: `classDiagram
      class WicTextureLoader {
        +Initialize() bool
        +DecodeFromFile() bool
        +CreateTextureFromDecodedImage() bool
        +LoadFromFile() bool
      }
      WicTextureLoader --> D3D11Renderer`, links: { D3D11Renderer: "d3d11renderer" }, scripts: [], source: code("Client/Rendering/WicTextureLoader.cpp", "bool WicTextureLoader::DecodeFromFile", ["WicTextureLoader::DecodeFromFile", "WicTextureLoader::CreateTextureFromDecodedImage"]) },
  diagnosticsoverlay: { type: "class", title: "DiagnosticsOverlay", summary: "D2D/DirectWrite로 엔진 지표를 백버퍼 위에 표시합니다.", intent: "측정 결과를 실행 중 즉시 관찰합니다.", issue: "도구 자체가 장면 GPU 측정을 오염시키면 안 됩니다.", final: "장면 timestamp 종료 후 반투명 panel과 text를 렌더링합니다.", next: "device lost 시 target recreate 자동화가 필요합니다.", graph: `classDiagram
      class DiagnosticsOverlay {
        +Initialize() bool
        +Draw(text) void
      }
      DiagnosticsOverlay --> D3D11Renderer`, links: { D3D11Renderer: "d3d11renderer" }, scripts: [], source: code("Client/Rendering/DiagnosticsOverlay.cpp", "bool DiagnosticsOverlay::Initialize", ["DiagnosticsOverlay::Initialize", "DiagnosticsOverlay::Draw"]) },
  processmemorysampler: { type: "class", title: "ProcessMemorySampler", summary: "Windows 프로세스 메모리 카운터를 snapshot으로 반환합니다.", intent: "OS 실제 지표와 엔진 추정치를 분리합니다.", issue: "Working Set 감소는 해제를 의미하지 않을 수 있습니다.", final: "GetProcessMemoryInfo로 Working/Peak/Private를 초당 한 번 수집합니다.", next: "commit charge와 GPU adapter budget을 함께 볼 수 있습니다.", graph: `classDiagram
      class ProcessMemorySampler {
        +Capture() ProcessMemorySnapshot
      }
      class ProcessMemorySnapshot {
        +workingSetBytes
        +peakWorkingSetBytes
        +privateBytes
      }
      ProcessMemorySampler --> TextureResourceManager
      ProcessMemorySampler --> D3D11Renderer`, links: { TextureResourceManager: "textureresourcemanager", D3D11Renderer: "d3d11renderer" }, scripts: [], source: code("Client/Diagnostics/ProcessMemorySampler.cpp", "ProcessMemorySampler::Capture", ["ProcessMemorySampler::Capture"]) },
  frametimeprofiler: { type: "class", title: "FrameTimeProfiler", summary: "고정 600샘플에서 percentile과 hitch count를 계산합니다.", intent: "평균에 숨는 tail latency를 관찰합니다.", issue: "측정 창 크기와 reset 시점이 결과 해석에 영향을 줍니다.", final: "고정 배열 ring buffer로 allocation 없이 최근 구간을 유지합니다.", next: "histogram과 percentile 근사 알고리즘 비교가 가능합니다.", graph: `classDiagram
      class FrameTimeProfiler {
        +RecordFrame()
        +RecordMilliseconds()
        +CaptureSnapshot()
        +Reset()
      }
      FrameTimeProfiler --> D3D11Renderer`, links: { D3D11Renderer: "d3d11renderer" }, scripts: [], source: code("Client/Diagnostics/FrameTimeProfiler.cpp", "void FrameTimeProfiler::RecordFrame", ["FrameTimeProfiler::RecordFrame", "FrameTimeProfiler::CaptureSnapshot"]) },
  renderingstressscene: { type: "class", title: "RenderingStressScene", summary: "동일한 스프라이트 배치를 100~10,000개로 생성합니다.", intent: "제출 방식만 바뀌는 통제된 부하를 제공합니다.", issue: "게임플레이 개체와 섞이면 비교 변수가 늘어납니다.", final: "진단 전용 RenderItem 배열을 별도로 생성합니다.", next: "분포·overdraw·texture variation 시나리오를 추가할 수 있습니다.", graph: `classDiagram
      class RenderingStressScene {
        +SetInstanceCount()
        +Disable()
        +RenderItems()
      }
      RenderingStressScene --> D3D11Renderer`, links: { D3D11Renderer: "d3d11renderer" }, scripts: [], source: code("Client/Diagnostics/RenderingStressScene.cpp", "void RenderingStressScene::SetInstanceCount", ["RenderingStressScene::SetInstanceCount"]) },
  benchmarksession: { type: "class", title: "BenchmarkSession", summary: "warmup·measure 단계와 시나리오 전환을 자동화합니다.", intent: "수동 타이밍 편향을 줄이고 CSV 재현성을 확보합니다.", issue: "VSync와 sample generation이 섞이면 이전 query가 결과를 오염시킬 수 있습니다.", final: "시나리오별 generation과 고정 sample 조건을 사용합니다.", next: "CI 환경 자동 실행이 다음 단계입니다.", graph: `classDiagram
      class BenchmarkSession {
        +Start()
        +Update()
        +CurrentScenario()
        +Stop()
      }
      BenchmarkSession --> RenderingStressScene
      BenchmarkSession --> FrameTimeProfiler`, links: { RenderingStressScene: "renderingstressscene", FrameTimeProfiler: "frametimeprofiler" }, scripts: [], source: code("Client/Diagnostics/BenchmarkSession.cpp", "void BenchmarkSession::Start", ["BenchmarkSession::Start", "BenchmarkSession::Update"]) },
  spatialgrid: { type: "class", title: "SpatialGrid", summary: "월드 좌표를 정수 cell key로 매핑하고 영역 후보를 반환합니다.", intent: "공격마다 전체 몬스터를 순회하지 않습니다.", issue: "음수 좌표에서도 truncation이 아니라 floor가 필요합니다.", final: "AABB가 걸치는 min/max cell 범위를 순회합니다.", next: "동적 update 비용을 계측할 수 있습니다.", graph: `flowchart LR
      GRID[SpatialGrid<br/>Rebuild · QueryAabb] --> COMBAT[CombatSystem<br/>Broad / Narrow Phase]`, links: { COMBAT: "combatsystem" }, scripts: [], source: code("Client/Gameplay/SpatialGrid.cpp", "void SpatialGrid::Rebuild", ["SpatialGrid::Rebuild", "SpatialGrid::QueryAabb"]) },
  combatsystem: { type: "class", title: "CombatSystem", summary: "broad-phase 후보에 정확한 거리·방향 판정을 적용합니다.", intent: "후보 수집과 피격 규칙을 분리합니다.", issue: "broad phase만으로 hit를 확정하면 false positive가 생깁니다.", final: "거리 제곱과 dot product를 사용해 sqrt 없이 판정합니다.", next: "공격 shape 전략 객체로 확장할 수 있습니다.", graph: `flowchart LR
      COMBAT[CombatSystem<br/>AttackCircle · AttackCone] --> GRID[SpatialGrid<br/>Rebuild · QueryAabb]`, links: { GRID: "spatialgrid" }, scripts: [], source: code("Client/Gameplay/CombatSystem.cpp", "CombatSystem::", ["CombatSystem::Attack", "CombatSystem::ConeAttack"]) },
  demoscene: { type: "class", title: "DemoScene", summary: "이동·점프·전투방과 RenderItem 생성을 연결합니다.", intent: "엔진 기능을 확인할 최소 플레이 환경입니다.", issue: "콘텐츠 확장보다 기술 검증 범위가 우선입니다.", final: "3개 방, 공격 이펙트 풀, 서버 위치 보정을 통합합니다.", next: "animation state와 data-driven room 정의가 남아 있습니다.", graph: `classDiagram
      class DemoScene {
        +Update()
        +RenderItems()
        +ReconcilePlayerPosition()
      }
       DemoScene *-- CombatSystem`, links: { CombatSystem: "combatsystem" }, scripts: [], source: code("Client/Presentation/DemoScene.cpp", "void DemoScene::Update", ["DemoScene::Update", "DemoScene::ReconcilePlayerPosition"]) },
  tcpclient: { type: "class", title: "TcpClient", summary: "SOCKET 소유권과 send lifecycle을 RAII로 관리합니다.", intent: "조기 반환과 오류에서도 소켓을 누수하지 않습니다.", issue: "복사되면 동일 SOCKET을 두 객체가 닫을 수 있습니다.", final: "복사를 삭제하고 Connect/Send/Disconnect의 소유권을 한 객체에 둡니다.", next: "partial send queue와 reconnect policy가 필요합니다.", graph: `classDiagram
      class TcpClient {
        +Connect() bool
        +Send() bool
        +Disconnect()
      }
      TcpClient --> TcpListener`, links: { TcpListener: "tcplistener" }, scripts: [], source: code("Client/Network/TcpClient.cpp", "bool TcpClient::Connect", ["TcpClient::Connect", "TcpClient::Send", "TcpClient::Disconnect"]) },
  serverstatereceiver: { type: "class", title: "ServerStateReceiver", summary: "수신 스레드가 최신 snapshot을 게시하고 안전하게 종료됩니다.", intent: "blocking recv가 메인 루프를 멈추지 않게 합니다.", issue: "Stop flag만 바꾸면 recv가 깨어나지 않아 join이 멈출 수 있습니다.", final: "shutdown으로 recv를 해제하고 worker를 join합니다.", next: "stream framing buffer와 malformed packet test가 필요합니다.", graph: `classDiagram
      class ServerStateReceiver {
        +Start() bool
        +TryConsumeLatest() bool
        +Stop()
      }
       ServerStateReceiver --> TcpClient`, links: { TcpClient: "tcpclient" }, scripts: [], source: code("Client/Network/ServerStateReceiver.cpp", "bool ServerStateReceiver::Start", ["ServerStateReceiver::Start", "ServerStateReceiver::TryConsumeLatest", "ServerStateReceiver::Stop"]) },
  packet: { type: "class", title: "Packet Contract", summary: "클라이언트와 서버가 공유하는 고정 크기 wire contract입니다.", intent: "패킷 크기·타입·sequence를 공통 정의합니다.", issue: "TCP byte stream에는 메시지 경계가 없습니다.", final: "현재 고정 크기 packet을 정확한 byte 수만큼 수신합니다.", next: "가변 payload에는 누적 버퍼와 header validation이 필요합니다.", graph: `classDiagram
       class PlayerMovePacket
       class PlayerStatePacket
       PlayerMovePacket --> TcpClient
       PlayerStatePacket --> TcpListener`, links: { TcpClient: "tcpclient", TcpListener: "tcplistener" }, scripts: [], source: code("Shared/Network/Packet.h", "struct PlayerMovePacket", ["PlayerMovePacket", "PlayerStatePacket"]) },
  tcplistener: { type: "class", title: "TcpListener", summary: "서버의 bind·listen·accept 수명을 관리합니다.", intent: "네트워크 시작 실패와 세션 종료를 명확히 처리합니다.", issue: "현재 단일 클라이언트 실험 범위입니다.", final: "Winsock runtime 이후 listener를 만들고 한 세션의 packet loop를 수행합니다.", next: "다중 session dispatcher와 IOCP가 확장 방향입니다.", graph: `classDiagram
      class TcpListener {
        +Start() bool
        +Accept() SOCKET
        +Stop()
      }
      TcpListener --> PacketContract`, links: { PacketContract: "packet" }, scripts: [], source: code("Server/Network/TcpListener.cpp", "bool TcpListener::Start", ["TcpListener::Start", "TcpListener::Accept", "TcpListener::Stop"]) }
};

const navigationAliases = new Map([
  ["playermovepacket", "packet"],
  ["playerstatepacket", "packet"],
  ["tcpserver", "tcplistener"],
  ["clientmain", "application"],
  ["receiverthread", "serverstatereceiver"]
]);

const treeGroups = [
  { title: "프로젝트 개요", ids: ["overview"] },
  { title: "엔진 연구 항목", ids: ["renderer", "instancing", "streaming", "diagnostics", "memory", "combat", "network"] },
  { title: "렌더링 / 진단 시스템", ids: ["d3d11renderer", "asynctextureloader", "textureresourcemanager", "wictextureloader", "diagnosticsoverlay", "processmemorysampler", "frametimeprofiler", "renderingstressscene", "benchmarksession"] },
  { title: "게임플레이 / 네트워크", ids: ["application", "demoscene", "spatialgrid", "combatsystem", "tcpclient", "serverstatereceiver", "packet", "tcplistener"] }
];

const els = {
  layout: document.getElementById("layout"), tree: document.getElementById("tree"), treeSearch: document.getElementById("explorer-search"),
  title: document.getElementById("node-title"), summary: document.getElementById("node-summary"), graph: document.getElementById("graph"),
  graphWrap: document.getElementById("graph-wrap"), intent: document.getElementById("intent"), issue: document.getElementById("issue"),
  final: document.getElementById("final"), next: document.getElementById("next"), scripts: document.getElementById("script-list"),
  evidence: document.getElementById("evidence"), benchmark: document.getElementById("benchmark-report"), details: document.getElementById("detail-grid"),
  codePath: document.getElementById("code-path"), codeActions: document.getElementById("code-actions"), codePreview: document.getElementById("code-preview"),
  back: document.getElementById("nav-back"), resizer: document.getElementById("code-resizer")
};

let currentNodeId = "";
let navStack = [];
let graphScale = 1;
let graphBaseSize = { width: 720, height: 360 };
let graphPanState = null;
let suppressGraphClick = false;
let searchQuery = "";
const expanded = new Set(treeGroups.map((group) => group.title));

function renderTree() {
  els.tree.innerHTML = "";
  const query = searchQuery.trim().toLowerCase();
  let visibleCount = 0;
  treeGroups.forEach((group) => {
    const ids = group.ids.filter((id) => !query || nodes[id].title.toLowerCase().includes(query) || nodes[id].summary.toLowerCase().includes(query));
    if (!ids.length) return;
    visibleCount += ids.length;
    const wrapper = document.createElement("section"); wrapper.className = "tree-group";
    const title = document.createElement("button"); title.className = "tree-title"; title.type = "button"; title.textContent = group.title;
    title.setAttribute("aria-expanded", String(query || expanded.has(group.title)));
    const items = document.createElement("div"); items.className = "tree-group-items"; items.hidden = !(query || expanded.has(group.title));
    title.addEventListener("click", () => { expanded.has(group.title) ? expanded.delete(group.title) : expanded.add(group.title); renderTree(); });
    ids.forEach((id) => {
      const button = document.createElement("button"); button.type = "button"; button.className = `tree-item ${nodes[id].type === "class" ? "child" : ""}`;
      button.dataset.id = id; button.textContent = nodes[id].title; if (id === currentNodeId) button.classList.add("active");
      button.addEventListener("click", () => selectNode(id)); items.appendChild(button);
    });
    wrapper.append(title, items); els.tree.appendChild(wrapper);
  });
  if (!visibleCount) els.tree.innerHTML = '<p class="tree-empty">검색 결과가 없습니다.</p>';
}

async function selectNode(id, options = {}) {
  const node = nodes[id]; if (!node) return;
  if (options.pushHistory !== false && currentNodeId && currentNodeId !== id) navStack.push(currentNodeId);
  currentNodeId = id; renderTree(); els.back.disabled = navStack.length === 0;
  els.title.textContent = node.title; els.summary.textContent = node.summary;
  els.intent.textContent = node.intent; els.issue.textContent = node.issue; els.final.textContent = node.final; els.next.textContent = node.next;
  renderBenchmark(node.report); renderScripts(node); renderEvidence(node); await renderGraph(node);
  if (node.source) await loadCode(node.source); else { els.codePath.textContent = ""; els.codeActions.innerHTML = ""; els.codePreview.innerHTML = "<code>시스템의 핵심 소스에서 클래스를 선택하세요.</code>"; }
}

function renderBenchmark(kind) {
  els.benchmark.hidden = !kind; els.layout.classList.toggle("benchmark-mode", Boolean(kind));
  if (!kind) { els.benchmark.innerHTML = ""; return; }
  const common = `<div class="benchmark-kpis">
    <div class="benchmark-kpi"><strong>57.416 → 6.798ms</strong><span>동일 7장 로딩의 Frame Max</span></div>
    <div class="benchmark-kpi"><strong>10,002 → 3</strong><span>스프라이트 10,000개의 Draw Call</span></div>
    <div class="benchmark-kpi"><strong>0 / 600</strong><span>비동기 로딩 후 16.67ms 초과</span></div>
    <div class="benchmark-kpi"><strong>24.50 MiB</strong><span>텍스처+인스턴스 추적 GPU</span></div>
  </div>`;
  const notes = {
    overview: "구현 → 계측 → 비교 → 한계 명시의 순서로 엔진 기능을 검증했습니다.",
    rendering: "FPS가 Present에 가려질 때도 CPU/GPU/Present 구간을 분리해 제출 비용을 확인합니다.",
    streaming: "총 로딩 시간은 비슷하지만 메인 스레드의 단일 프레임 정지를 제거하는 것이 목표입니다."
  };
  els.benchmark.innerHTML = `<div class="benchmark-hero"><p class="benchmark-kicker">측정 결과</p><h2>${nodes[currentNodeId].title}</h2><p>${notes[kind]}</p></div>${common}<div class="benchmark-note">Release · Windows x64 · 최근 600프레임 측정 구간. 수치는 현재 장비의 결과이며 전체 하드웨어에 일반화하지 않습니다.</div>`;
}

function renderScripts(node) {
  els.scripts.innerHTML = "";
  (node.scripts || []).forEach((id) => {
    const target = nodes[id]; if (!target) return;
    const card = document.createElement("article"); card.className = "script-card";
    const button = document.createElement("button"); button.type = "button"; button.textContent = target.title; button.addEventListener("click", () => selectNode(id));
    const p = document.createElement("p"); p.textContent = target.summary; card.append(button, p); els.scripts.appendChild(card);
  });
  document.getElementById("script-section").hidden = !(node.scripts || []).length;
}

function renderEvidence(node) {
  els.evidence.innerHTML = "";
  (node.evidence || []).forEach((id) => {
    const item = evidence[id]; if (!item) return;
    const button = document.createElement("button"); button.type = "button"; button.className = "evidence-item";
    button.innerHTML = `<img src="${item.src}" alt="${item.caption}" loading="lazy"><span>${item.caption}</span>`;
    button.addEventListener("click", () => openMedia(item)); els.evidence.appendChild(button);
  });
  document.getElementById("evidence-section").hidden = !(node.evidence || []).length;
}

async function renderGraph(node) {
  els.graphWrap.dataset.diagram = node.graph.startsWith("classDiagram") ? "class" : "flow";
  els.graph.className = "mermaid";
  els.graph.removeAttribute("data-processed"); els.graph.textContent = node.graph;
  try {
    await mermaid.run({ nodes: [els.graph] });
    captureGraphBaseSize(node);
    graphScale = 1;
    applyGraphScale();
    fitGraphContainer();
    resetGraphPan();
    attachGraphClicks(node);
  }
  catch (error) { els.graph.textContent = `다이어그램을 표시하지 못했습니다: ${error.message}`; }
}

function attachGraphClicks(node) {
  const svg = els.graph.querySelector("svg"); if (!svg) return;
  const knownTitles = new Map(Object.entries(nodes).map(([id, item]) => [normalizeGraphLabel(item.title), id]));
  svg.querySelectorAll("g.node, g.classGroup, g.actor").forEach((group) => {
    const rawId = normalizeGraphLabel((group.id || "").replace(/^flowchart-/, "").replace(/^classId-/, "").replace(/-\d+$/, ""));
    const label = graphGroupLabel(group);
    const target = resolveGraphTarget(node, knownTitles, rawId, label);
    if (!target || target === currentNodeId) return;
    group.classList.add("graph-clickable");
    group.addEventListener("click", () => {
      if (!suppressGraphClick) selectNode(target);
    });
  });

  svg.querySelectorAll("text.actor").forEach((actor) => {
    const group = actor.parentElement;
    const target = resolveGraphTarget(node, knownTitles, "", actor.textContent.trim());
    if (!group || !target || target === currentNodeId) return;
    group.classList.add("graph-clickable");
    group.addEventListener("click", () => {
      if (!suppressGraphClick) selectNode(target);
    });
  });

  const methods = node.source?.methods || [];
  if (!methods.length) return;
  svg.querySelectorAll("text, span").forEach((element) => {
    const label = element.textContent.trim();
    const method = methods.find((item) => shortMethodName(item) === shortMethodName(label));
    if (!method) return;
    element.classList.add("graph-method-clickable");
    element.addEventListener("click", (event) => {
      event.stopPropagation();
      jumpToMethod(method);
    });
  });
}

function resolveGraphTarget(node, knownTitles, rawId, label) {
  const normalizedLabel = normalizeGraphLabel(label);
  return Object.entries(node.links || {}).find(([key]) => normalizeGraphLabel(key) === rawId)?.[1]
    || Object.entries(node.links || {}).find(([key]) => normalizedLabel.includes(normalizeGraphLabel(key)))?.[1]
    || knownTitles.get(normalizedLabel)
    || navigationAliases.get(normalizedLabel);
}

function graphGroupLabel(group) {
  const classTitle = group.querySelector(".classTitle, .classTitleText");
  return (classTitle?.textContent || group.querySelector(".nodeLabel")?.textContent || group.textContent || "").trim();
}

function normalizeGraphLabel(value) {
  return String(value).replace(/[+:#()~<>]/g, " ").replace(/\s+/g, " ").trim().toLowerCase();
}

function shortMethodName(value) {
  return String(value).replace(/^.*::/, "").replace(/^\+/, "").replace(/\s.*$/, "").replace(/\(.*/, "").trim();
}

async function loadCode(source) {
  els.codePath.textContent = source.path; els.codeActions.innerHTML = "";
  source.methods.forEach((method) => { const b = document.createElement("button"); b.type = "button"; b.textContent = method; b.addEventListener("click", () => jumpToMethod(method)); els.codeActions.appendChild(b); });
  try { const response = await fetch(source.path); if (!response.ok) throw new Error(`${response.status}`); renderSource(await response.text(), source.focus); }
  catch (error) { els.codePreview.innerHTML = `<code>소스를 불러오지 못했습니다: ${escapeHtml(error.message)}\n로컬에서는 HTTP 서버로 실행하세요.</code>`; }
}

function renderSource(source, focus) {
  const lines = source.replace(/\r/g, "").split("\n");
  els.codePreview.innerHTML = lines.map((line, i) => `<span class="code-line" data-line="${i + 1}"><span class="line-no">${String(i + 1).padStart(4, " ")}</span>${highlight(line)}</span>`).join("");
  if (focus) jumpToMethod(focus);
}

function jumpToMethod(needle) {
  const rows = [...els.codePreview.querySelectorAll(".code-line")]; const row = rows.find((item) => item.textContent.includes(needle));
  rows.forEach((item) => item.classList.remove("hit"));
  if (row) {
    row.classList.add("hit");
    els.codePreview.scrollTo({ top: Math.max(0, row.offsetTop - els.codePreview.clientHeight * 0.45), behavior: "smooth" });
  }
}

function highlight(line) {
  let value = escapeHtml(line);
  value = value.replace(/(&quot;.*?&quot;)/g, '<span class="str">$1</span>');
  value = value.replace(/\b(class|struct|namespace|public|private|const|constexpr|void|bool|return|if|else|for|while|auto|static|noexcept|true|false|nullptr)\b/g, '<span class="kw">$1</span>');
  value = value.replace(/(\/\/.*)$/g, '<span class="com">$1</span>'); return value;
}

function escapeHtml(value) { return String(value).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;"); }
function openMedia(item) { const dialog = document.getElementById("media-modal"); const image = document.getElementById("modal-image"); image.src = item.src; image.alt = item.caption; document.getElementById("modal-caption").textContent = item.caption; dialog.showModal(); }

function captureGraphBaseSize(node) {
  const svg = els.graph.querySelector("svg");
  if (!svg) return;
  const viewBox = svg.viewBox.baseVal;
  const naturalWidth = viewBox?.width || svg.getBBox().width || 720;
  const naturalHeight = viewBox?.height || svg.getBBox().height || 360;
  const isClass = node.graph.startsWith("classDiagram");
  const isSequence = node.graph.startsWith("sequenceDiagram");
  const width = isClass ? clamp(naturalWidth, 240, 420)
    : isSequence ? clamp(naturalWidth * 0.9, 640, 920)
      : clamp(naturalWidth * 0.68, 420, 680);
  graphBaseSize = { width, height: Math.max(170, width * naturalHeight / naturalWidth) };
}

function applyGraphScale() {
  const svg = els.graph.querySelector("svg");
  if (!svg) return;
  const width = Math.round(graphBaseSize.width * graphScale);
  const height = Math.round(graphBaseSize.height * graphScale);
  svg.style.width = `${width}px`;
  svg.style.height = `${height}px`;
  els.graph.style.width = `${width}px`;
  els.graph.style.height = `${height}px`;
  document.getElementById("zoom-reset").textContent = `${Math.round(graphScale * 100)}%`;
}

function setGraphScale(value, anchor) {
  const previous = graphScale;
  graphScale = clamp(value, 0.55, 2.2);
  if (graphScale === previous) return;
  const point = anchor || { x: els.graphWrap.clientWidth / 2, y: els.graphWrap.clientHeight / 2 };
  const contentX = els.graphWrap.scrollLeft + point.x;
  const contentY = els.graphWrap.scrollTop + point.y;
  applyGraphScale();
  const ratio = graphScale / previous;
  els.graphWrap.scrollLeft = contentX * ratio - point.x;
  els.graphWrap.scrollTop = contentY * ratio - point.y;
}

function clamp(value, min, max) { return Math.max(min, Math.min(max, value)); }
function fitGraphContainer() {
  const preferredHeight = Math.ceil(graphBaseSize.height + 52);
  els.graphWrap.style.height = `${clamp(preferredHeight, 220, 460)}px`;
}

function resetGraphPan() {
  requestAnimationFrame(() => {
    els.graphWrap.scrollLeft = Math.max(0, (els.graphWrap.scrollWidth - els.graphWrap.clientWidth) / 2);
    els.graphWrap.scrollTop = 0;
  });
}

document.getElementById("zoom-out").addEventListener("click", () => setGraphScale(graphScale - 0.15));
document.getElementById("zoom-in").addEventListener("click", () => setGraphScale(graphScale + 0.15));
document.getElementById("zoom-reset").addEventListener("click", () => setGraphScale(1));
els.graphWrap.addEventListener("wheel", (event) => {
  if (!event.ctrlKey) return;
  event.preventDefault();
  const bounds = els.graphWrap.getBoundingClientRect();
  setGraphScale(graphScale * Math.exp(-event.deltaY * 0.0012), { x: event.clientX - bounds.left, y: event.clientY - bounds.top });
}, { passive: false });
els.graphWrap.addEventListener("pointerdown", (event) => {
  if (event.button !== 0) return;
  graphPanState = { pointerId: event.pointerId, x: event.clientX, y: event.clientY, left: els.graphWrap.scrollLeft, top: els.graphWrap.scrollTop, moved: false };
  els.graphWrap.setPointerCapture(event.pointerId);
});
els.graphWrap.addEventListener("pointermove", (event) => {
  if (!graphPanState || graphPanState.pointerId !== event.pointerId) return;
  const dx = event.clientX - graphPanState.x;
  const dy = event.clientY - graphPanState.y;
  if (Math.abs(dx) + Math.abs(dy) > 4) graphPanState.moved = true;
  if (!graphPanState.moved) return;
  els.graphWrap.classList.add("is-panning");
  els.graphWrap.scrollLeft = graphPanState.left - dx;
  els.graphWrap.scrollTop = graphPanState.top - dy;
});
function finishGraphPan(event) {
  if (!graphPanState || graphPanState.pointerId !== event.pointerId) return;
  const moved = graphPanState.moved;
  graphPanState = null;
  els.graphWrap.classList.remove("is-panning");
  if (els.graphWrap.hasPointerCapture(event.pointerId)) els.graphWrap.releasePointerCapture(event.pointerId);
  if (moved) { suppressGraphClick = true; setTimeout(() => { suppressGraphClick = false; }, 0); }
}
els.graphWrap.addEventListener("pointerup", finishGraphPan);
els.graphWrap.addEventListener("pointercancel", finishGraphPan);
els.back.addEventListener("click", () => { const previous = navStack.pop(); if (previous) selectNode(previous, { pushHistory: false }); });
els.treeSearch.addEventListener("input", (event) => { searchQuery = event.target.value; renderTree(); });
const themeToggle = document.getElementById("theme-toggle");
function applyTheme(theme) {
  const dark = theme === "dark";
  document.body.classList.toggle("dark-theme", dark);
  themeToggle.textContent = dark ? "Light" : "Night";
  themeToggle.title = dark ? "Light 테마로 전환" : "Night 테마로 전환";
  themeToggle.setAttribute("aria-label", themeToggle.title);
  themeToggle.setAttribute("aria-pressed", String(dark));
}
applyTheme(localStorage.getItem("dungeonsync-theme") === "dark" ? "dark" : "light");
themeToggle.addEventListener("click", () => {
  const theme = document.body.classList.contains("dark-theme") ? "light" : "dark";
  localStorage.setItem("dungeonsync-theme", theme);
  applyTheme(theme);
});
document.getElementById("modal-close").addEventListener("click", () => document.getElementById("media-modal").close());

const defaultCodePanelWidth = 460;
function setCodePanelWidth(width) { els.layout.style.setProperty("--code-panel-width", `${Math.max(320, Math.min(820, width))}px`); }
els.resizer.addEventListener("pointerdown", (event) => { els.resizer.setPointerCapture(event.pointerId); els.layout.classList.add("is-resizing"); });
els.resizer.addEventListener("pointermove", (event) => { if (els.layout.classList.contains("is-resizing")) setCodePanelWidth(window.innerWidth - event.clientX); });
const finishResize = (event) => { if (els.layout.classList.contains("is-resizing")) { els.layout.classList.remove("is-resizing"); if (els.resizer.hasPointerCapture(event.pointerId)) els.resizer.releasePointerCapture(event.pointerId); } };
els.resizer.addEventListener("pointerup", finishResize); els.resizer.addEventListener("pointercancel", finishResize); els.resizer.addEventListener("dblclick", () => setCodePanelWidth(defaultCodePanelWidth));

mermaid.initialize({
  startOnLoad: false,
  securityLevel: "loose",
  theme: "base",
  themeVariables: { fontSize: "13px", fontFamily: "Segoe UI, Noto Sans KR, Arial, sans-serif" },
  flowchart: { curve: "basis", htmlLabels: true, nodeSpacing: 24, rankSpacing: 34, padding: 8 },
  sequence: { actorMargin: 30, messageMargin: 24, diagramMarginX: 20, diagramMarginY: 16 }
});
selectNode("overview", { pushHistory: false });

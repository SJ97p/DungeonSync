# DungeonSync Publication Review

## 예정 공개 주소

- Repository: `https://github.com/SJ97p/DungeonSync`
- GitHub Pages: `https://sj97p.github.io/DungeonSync/`

저장소 이름을 변경하면 `README.md`, `index.html`의 링크를 함께 수정해야 합니다.

## GitHub Pages 구조

저장소 루트의 다음 파일을 정적 사이트로 사용합니다.

- `index.html`
- `style.css`
- `app.js`
- `assets/evidence/*`

별도 npm 빌드 과정은 없습니다. GitHub Pages Source를 `Deploy from a branch`, Branch를 `main`, Folder를 `/(root)`로 설정합니다.

## 공개 범위

- C++20 클라이언트·서버 소스
- HLSL shader
- CMake configuration
- benchmark CSV와 결과 그래프
- 엔진 진단 스크린샷
- 설계·요구사항 문서

## 공개 제외

- `build/`, `out/`, `.vs/`
- `.exe`, `.dll`, `.pdb`, `.obj` 등 빌드 산출물
- 로컬 로그와 임시 측정 파일
- API key, token, password, 개인 환경 설정

## 게시 전 확인

- [ ] Debug/Release build 성공
- [ ] CodeMap Explorer, Mermaid graph, source preview 정상
- [ ] README와 Pages의 수치가 Release 로그와 일치
- [ ] `Benchmarks/results.csv`와 날짜별 원본 CSV의 공개 필요성 결정
- [ ] `git diff --cached`로 실제 게시 파일 최종 확인
- [ ] GitHub Pages 주소 접속 후 대소문자 경로 확인
- [ ] repository About에 C++, DirectX 11, performance profiling, networking 기재

## 표현 원칙

- 기존 라이브 레거시를 개선했다고 주장하지 않는다.
- 독립 연구 환경에서 병목을 재현하고 개선한 결과로 설명한다.
- `Tracked GPU`를 전체 VRAM 사용량으로 표현하지 않는다.
- 절대 수치를 모든 하드웨어에 일반화하지 않는다.
- 단일 클라이언트 TCP 실험을 대규모 서버 구현으로 표현하지 않는다.

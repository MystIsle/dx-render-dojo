# dx-render-dojo

렌더러 공부방. Rastertek DirectX 11 튜토리얼을 "원문은 스펙, 코드는 내 것" 방식으로 다시 쓰는 학습 리포입니다.

## 원칙

- 학습이 목적이고 엔진은 목적이 아님. 진도가 지표
- 현재 튜토리얼이 요구하지 않는 추상화는 만들지 않음. 중복은 3번째에서 뽑음
- 코드는 100% Windows 코드. mingw 크로스컴파일 호환 유지 (맥에서는 Wine 으로 실행)
- 튜토리얼 한 편 = 단계 커밋 → 단계마다 Windows 빌드·실행 → 태그 → 문서 생성 → 산문 검토. 맥 빌드는 태그 조건이 아님
- 태그 : 원문 편은 `tutNN`, 원문에 없는 편은 `tutalpha01` 형식. 따라 하기 N단계 커밋은 `tutNN-sN` (`docs/DOC_STYLE.md` 2절)
- 작업 브랜치는 `work/tutNN`. 편이 끝나 태그를 달면 지움. 태그와 이름이 겹치면 git 이 어느 쪽인지 모른다고 경고함
- 스테이지에는 태그·브랜치를 달지 않음. 스테이지와 편의 대응은 목차에만 두고, 스테이지 끝은 그 스테이지 마지막 편의 `tutNN` 으로 가리킴

## 코드를 쓰기 전에

- `docs/CODING_STYLE.md` : 코딩 규약의 원본. 규약을 바꿀 때는 이 문서를 고침
- `.clang-format` : 서식
- `.clang-tidy` : 네이밍. 도구가 못 잡는 항목은 CODING_STYLE.md 6절에 있음
- `DxRenderDojo.sln.DotSettings` : Rider 이름 규칙. CODING_STYLE.md 6절을 옮긴 것이라 6절을 먼저 고치고 Rider 설정에서 맞춤

## 교재 문서를 쓰기 전에

- `docs/DOC_STYLE.md` : 교재 문서 규약의 원본. 규약을 바꿀 때는 이 문서를 고침
- `tools/doc-mock/toc.json` : 목차. 스테이지·편 제목·앞 편 태그의 원본. 빌드가 원고 머리의 STAGE·이전 태그, 이전·다음 편, 목차 페이지(`index.html`)를 여기서 채움
- 원고는 편마다 `tools/doc-mock/template-NN.html`. `build.ps1 -Lesson NN` 이 태그 코드를 채워 `tutorial-NN.html` 을 만들고, 셀 수 있는 규칙은 `tools/doc-mock/check_prose.py` 가 봄

## 코드 구조

`Source/` 를 include 기준으로 두고, include 는 항상 폴더까지 적음 (`#include "Core/FSystem.h"`). 짝 헤더도 같음. 근거는 `docs/CODING_STYLE.md` 7절

- `Source/Main.cpp` : `wWinMain`
- `Source/Core/` : 창·메시지 루프·입력·설정. D3D 를 부르는 곳은 `FSystem` 이 프레임 앞뒤와 크기 변경에서 `FD3D11Graphics` 를 부르는 자리뿐
- `Source/App/` : 편마다 바뀌는 장면 코드 (`FApplication`)
- `Source/Graphics/` : 03편부터. `FD3D11Graphics`(디바이스·스왑체인·파이프라인 전역 상태)·카메라·모델
- `Source/Shaders/` : 04편부터. HLSL 파일(`.vs`·`.ps`, BOM 없음)과 그것을 감싸는 C++ 클래스. 빌드가 HLSL 파일을 exe 옆 `Shaders/` 로 복사하고, 코드는 exe 폴더 기준으로 읽음
- `Source/Utility/` : 02편부터. 공통 도구. 로그(`FLog`)·검사 매크로(`Check.h`)·싱글톤 틀(`TSingleton`)·실행 파일 기준 경로(`FPaths`)
- `external/DirectXMath/` : 공식 DirectXMath 릴리스의 `Inc/` 헤더. MSVC·mingw 가 같은 사본을 씀. 버전을 올릴 때는 통째로 바꿈

새 소스는 `DxRenderDojo.vcxproj`(+ `.vcxproj.filters`)에 등록. `CMakeLists.txt` 는 `Source/` 아래 `.cpp` 를 모으므로 적지 않음

셰이더 파일은 vcxproj 에서 항목 형식을 "파일 복사"(대상 디렉터리 `$(OutDir)Shaders`)로 둠. `CMakeLists.txt` 는 `Source/Shaders` 의 `.vs`·`.ps` 를 모아 복사함

## 빌드와 검사

- Visual Studio 빌드 : `DxRenderDojo.sln`. x64 전용
- 도구 집합은 v143 을 유지함. 언리얼 엔진 작업의 주력 도구 집합이 v143 이라서임. Visual Studio 2026 의 기본(v145)으로 올리지 않음
- 검사용 CMake 빌드 : VS 개발자 명령 프롬프트(`vcvars64.bat`)에서 실행. `build-win/compile_commands.json` 이 생겨 clang-tidy 가 컴파일 옵션을 그대로 씀

```
cmake -B build-win -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-win
clang-format --dry-run -Werror <파일>
clang-tidy -p build-win <파일>
```

- clang-tidy 는 20 이상을 씀. VS 2022 에 들어 있는 19.1.5 는 VS 18 표준 라이브러리 헤더가 거부함 (VS 18 의 LLVM 은 22.1.3)
- 맥 : `./scripts/mac-build.sh` 로 빌드, `./scripts/mac-run.sh` 로 실행(Wine), `./scripts/mac-lint.sh [파일...]` 로 검사. 검사는 Homebrew `llvm` 이 필요하고, 인자가 없으면 `Source/` 전체를 봄
- 맥의 clang-tidy 는 mingw 타깃·sysroot·`-std=c++20` 을 따로 넘겨야 헤더를 찾음. 인자는 `mac-lint.sh` 에 있음

## 인코딩·줄끝

- 모든 텍스트 : BOM 없는 UTF-8 (`.editorconfig`)
- 셰이더 `.hlsl` `.vs` `.ps` : BOM 이 있으면 `D3DCompileFromFile` 이 `(1,1): error X3000` 으로 실패
- C++ : BOM 이 없으므로 프로젝트 파일(vcxproj·CMake)에 `/utf-8` 필수. 없으면 MSVC 가 한글을 CP949 로 읽음
- Rider 설정 `DxRenderDojo.sln.DotSettings` : BOM 있는 UTF-8. Rider 가 저장할 때마다 붙이므로 예외로 둠
- 줄끝 : `.gitattributes`. C++·셰이더·sln·vcxproj 는 CRLF, sh·cmake·md·json·xml·DotSettings 는 LF

## 자주 걸리는 곳

- 플립 모델은 `Present` 가 백 버퍼를 파이프라인에서 뗌. RTV 는 `BeginScene` 에서 매 프레임 묶음
- `ComPtr` 의 `&` 는 `ReleaseAndGetAddressOf()` 임. `OMSetRenderTargets(1, &Rtv, ...)` 처럼 넘기면 호출 직전에 RTV 가 해제돼 크래시. 주소는 항상 `.GetAddressOf()` 로 넘김
- 매니페스트는 빌드마다 넣는 길이 다름. 한 exe 에 두 번 들어가면 `CVT1100` 으로 링크 실패
  - MSVC(vcxproj) : 링커의 "추가 매니페스트 파일"(`<Manifest><AdditionalManifestFiles>`)
  - MSVC(CMake) : `.manifest` 를 `target_sources` 로 넘김. `/MANIFEST:EMBED` 나 `/MANIFESTINPUT` 을 `target_link_options` 로 직접 주면 CMake 의 `vs_link_exe` 가 만드는 `manifest.res` 와 겹쳐 실패함
  - mingw : `.rc` 에 `1 24 "<파일>"`. 이 `.rc` 는 MSVC 빌드에 넣지 않음
- mingw 호환
  - `#pragma comment(lib, ...)` 금지. 라이브러리는 CMake 의 `target_link_libraries` 에 추가
  - `sprintf_s` 등 `_s` 계열은 `#ifdef _MSC_VER` 로 분기하고 그 밖에서는 `snprintf`
  - `CD3D11_VIEWPORT`, `CD3D11_RECT`, `CD3D11_DEPTH_STENCIL_VIEW_DESC`, `D3D11_MIN_DEPTH`, `D3D11_MAX_DEPTH` 는 mingw 헤더에 없음. `CD3D11_BUFFER_DESC`, `CD3D11_TEXTURE2D_DESC`, `CD3D11_RASTERIZER_DESC`, `CD3D11_SAMPLER_DESC` 는 있음

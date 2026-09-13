# dx-render-dojo

렌더러 공부방. Rastertek DirectX 11 튜토리얼을 "원문은 스펙, 코드는 내 것" 방식으로 다시 쓰는 학습 리포입니다.

## 원칙

- 학습이 목적이고 엔진은 목적이 아님. 진도가 지표
- 현재 튜토리얼이 요구하지 않는 추상화는 만들지 않음. 중복은 3번째에서 뽑음
- 코드는 100% Windows 코드. mingw 크로스컴파일 호환 유지 (맥에서는 Wine 으로 실행)
- 튜토리얼 한 편 = 코드 → Windows 빌드 → 태그 → 문서 생성 → 산문 검토. 맥 빌드는 태그 조건이 아님
- 태그 : 원문 편은 `tutNN`, 원문에 없는 편은 `tutalpha01` 형식
- 스테이지에는 태그·브랜치를 달지 않음. 스테이지와 편의 대응은 목차에만 두고, 스테이지 끝은 그 스테이지 마지막 편의 `tutNN` 으로 가리킴

## 코드를 쓰기 전에

- `docs/CODING_STYLE.md` : 코딩 규약의 원본. 규약을 바꿀 때는 이 문서를 고침
- `.clang-format` : 서식
- `.clang-tidy` : 네이밍. 도구가 못 잡는 항목은 CODING_STYLE.md 6절에 있음

## 빌드와 검사

- Visual Studio 빌드 : `DxRenderDojo.sln`. x64 전용
- 검사용 CMake 빌드 : VS 개발자 명령 프롬프트(`vcvars64.bat`)에서 실행. `build-win/compile_commands.json` 이 생겨 clang-tidy 가 컴파일 옵션을 그대로 씀

```
cmake -B build-win -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-win
clang-format --dry-run -Werror <파일>
clang-tidy -p build-win <파일>
```

- clang-tidy 는 20 이상을 씀. VS 2022 에 들어 있는 19.1.5 는 VS 18 표준 라이브러리 헤더가 거부함 (VS 18 의 LLVM 은 22.1.3)

## 인코딩·줄끝

- 모든 텍스트 : BOM 없는 UTF-8 (`.editorconfig`)
- 셰이더 `.hlsl` `.vs` `.ps` : BOM 이 있으면 `D3DCompileFromFile` 이 `(1,1): error X3000` 으로 실패
- C++ : BOM 이 없으므로 프로젝트 파일(vcxproj·CMake)에 `/utf-8` 필수. 없으면 MSVC 가 한글을 CP949 로 읽음
- 줄끝 : `.gitattributes`. C++·셰이더·sln·vcxproj 는 CRLF, sh·cmake·md·json·xml 은 LF

## 자주 걸리는 곳

- `ComPtr` 의 `&` 는 `ReleaseAndGetAddressOf()` 임. `OMSetRenderTargets(1, &Rtv, ...)` 처럼 넘기면 호출 직전에 RTV 가 해제돼 크래시. 주소는 항상 `.GetAddressOf()` 로 넘김
- mingw 호환
  - `#pragma comment(lib, ...)` 금지. 라이브러리는 CMake 의 `target_link_libraries` 에 추가
  - `sprintf_s` 등 `_s` 계열은 `#ifdef _MSC_VER` 로 분기하고 그 밖에서는 `snprintf`
  - `CD3D11_VIEWPORT`, `CD3D11_RECT`, `CD3D11_DEPTH_STENCIL_VIEW_DESC`, `D3D11_MIN_DEPTH`, `D3D11_MAX_DEPTH` 는 mingw 헤더에 없음. `CD3D11_BUFFER_DESC`, `CD3D11_TEXTURE2D_DESC`, `CD3D11_RASTERIZER_DESC`, `CD3D11_SAMPLER_DESC` 는 있음

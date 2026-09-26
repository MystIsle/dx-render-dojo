# 인수 시험 도구

원고의 따라 하기 조각과 문장 안내만 옮겨서 이전 태그 코드가 이 편 태그 코드가 되는지 보는 시험(`docs/DOC_STYLE.md` 8절)에 쓰는 도구입니다. 시험하는 쪽이 기억으로 코드를 채우지 않도록 생성본에서 조각을 기계적으로 뽑아 넣습니다.

## 파일

- `dump_steps.py <tutorial-NN.html>` : 생성본의 따라 하기 절을 텍스트로 뽑음. `## STEP` 은 단계, `FOLD` 는 접기 제목, `CAPTION` 은 조각 캡션(배지와 넣을 자리), 코드 블록은 조각
- `apply.py` : 뽑은 조각을 worktree 에 넣음. 대상은 환경 변수 `ACC_WT`(worktree)와 `ACC_STEPS`(`dump_steps.py` 결과)
- `cmp_steps.py <옛.md> <새.md>` : 두 원고판의 조각·캡션·접기 제목을 비교함. 원고만 고친 판에 재시험이 필요한지 볼 때 씀
- `Probe.cs` : 실행 확인 스크립트가 불러 씀. `OutputDebugString` 로그 받기(DBWIN), 프로세스의 창 찾기, 창 크기 바꾸기, 메시지 보내기
- `probe02.ps1`·`probe03.ps1`·`probe04.ps1`·`shot.ps1` : 편별 실행 확인(아래)

## apply.py 동작

| 동작 | 쓰는 곳 |
|---|---|
| `list` · `show N` | 블록 번호·캡션 보기, 블록 하나 보기 |
| `write PATH N` | 파일 통째 조각 |
| `diff PATH N` | 범위 diff 조각. 문맥 줄과 지운 줄이 파일에서 한 곳에만 맞아야 함 |
| `func PATH N` | 함수 통째 교체. 첫 줄이 달라졌으면(반환형 교체 등) 함수 이름으로 찾음 |
| `append PATH N` | 파일 끝에 빈 줄 하나 두고 붙임 |
| `after PATH N ANCHOR` | ANCHOR 가 든 줄로 시작하는 중괄호 덩어리 뒤에 빈 줄 하나 두고 |
| `afterline PATH N ANCHOR` | ANCHOR 가 든 줄 바로 뒤 |
| `afterlineblank PATH N ANCHOR` | ANCHOR 가 든 줄 뒤에 빈 줄 하나 두고 (include 묶음 다음 등) |
| `aftercase PATH N ANCHOR` | ANCHOR case 의 `return 0;` 뒤에 빈 줄 하나 두고 |
| `move OLD NEW` | 파일 옮기기 문장 안내. 파일만 옮기고 내용은 뒤따르는 조각으로 고침 |

PowerShell 에서는 셸 상태가 이어지지 않을 수 있으니 명령마다 환경 변수를 붙입니다.

```
$env:ACC_WT='<시험 폴더>\wt'; $env:ACC_STEPS='<시험 폴더>\steps.md'; python -X utf8 tools\accept\apply.py list
```

## 절차

1. 이전 편 태그를 리포 밖 폴더에 풂 : `git worktree add --detach <시험 폴더>\wt tutMM`. 리포 작업 트리는 다른 작업이 쓰고 있을 수 있으니 건드리지 않음
2. 생성본 저장 : `cmd /c "git show <커밋>:tools/doc-mock/tutorial-NN.html > <파일>"`. PowerShell 의 `>` 는 인코딩을 바꿈
3. 조각 뽑기 : `python -X utf8 tools\accept\dump_steps.py <파일>` 의 출력을 `steps.md` 로
4. 단계 순서대로 적용
   - 캡션의 배지(신규·수정·삭제)와 넣을 자리로 동작을 고름. 접기 안 조각도 넣음
   - 제목이 "참고 … 따로 치지 않음" 인 접기는 넣지 않고 대조에만 씀
   - Visual Studio 조작(필터, 기존 항목 추가·제거, 속성)은 `.vcxproj`·`.vcxproj.filters` 를 VS 가 쓰는 XML 대로 손으로 고쳐 흉내 냄. 문장만으로 정할 수 없는 곳은 기록
   - 조각을 고치거나 빠진 코드를 채우지 않음. 판정 전에는 목표 태그의 소스를 보지 않음
5. 빌드 : MSBuild Debug·Release x64(`/m /nr:false /nologo /v:m`), CMake(`vcvars64.bat` 뒤 Ninja). 코드 경고 0. 시험 폴더가 임시 폴더 아래면 MSB8029·경로 길이 경고가 나는데 환경 경고라 따로 셈
6. 실행 확인 : 원고의 "다 됐는지 확인" 값과 대조. exe 는 `Start-Process` 로 띄움. 동기로 실행하면 창이 닫힐 때까지 명령이 멈춤
7. 판정
   - `git add -A -- <판정 경로> DxRenderDojo.vcxproj DxRenderDojo.vcxproj.filters`
   - `git diff --cached -w --ignore-blank-lines tutNN -- <판정 경로>` 가 비면 통과
   - 판정 경로 : `Source CMakeLists.txt DxRenderDojo.manifest DxRenderDojo.rc`, 04편부터 `external` 을 더함
   - 프로젝트 파일은 diff 가 아니라 빌드·실행 결과로 판정. `git diff --cached --stat tutNN` 으로 무엇이 다른지만 적음
8. 정리 : 떠 있는 exe 를 닫고 `git worktree remove --force <시험 폴더>\wt` 뒤 `git worktree prune`

## 단계 태그로 시험할 때

02편부터 따라 하기 N단계마다 커밋 `tutNN-sN` 이 있습니다. 단계 k 는 `tutNN-s(k-1)`(첫 단계는 이전 편 태그)에서 그 단계 조각만 넣고 `tutNN-sk` 와 비교하면 어긋난 단계를 바로 짚습니다. 이 방식으로는 아직 돌려 보지 않았습니다 [미확인].

## 여러 편을 나란히 시험할 때

- 편마다 시험 폴더와 `ACC_WT`·`ACC_STEPS` 를 따로 둠
- 로그 받기(DBWIN)는 시스템에 하나뿐이고 창 캡처도 겹치므로 exe 실행은 한 번에 하나. 실행 전에 `[IO.File]::Open('<공용 폴더>\run.lock', 'CreateNew')` 가 성공한 쪽만 띄우고, 끝나면 핸들을 닫고 파일을 지움

## 실행 확인 스크립트

- `probe02.ps1 -Exe <exe>` : 창 크기, 가운데 픽셀, 옮기기만 할 때 로그, 최소 크기 로그, ESC·닫기 종료 코드
- `probe03.ps1 -Exe <exe> [-Mode normal|fatal|resizefatal]` : 로그, 2초 CPU, 크기 변경, Alt+Enter 네 번과 원래 자리 복귀, ESC 종료 코드. `fatal` 은 치명 실패 상자의 제목·본문과 종료 코드
- `shot.ps1 -Exe <exe> -Png <파일>` : 창 캡처, 가운데 픽셀, 2초 CPU, ESC 종료 코드
- `probe04.ps1 -Exe <exe> [-Mode log|shot|quick] [-WorkDir <폴더>] [-OutDir <폴더>]` : `log` 는 안쪽 크기 1200×600·600×800·800×600 과 Alt+Enter 전환 로그, [D3D]·[DXGI] 경고 수. `shot` 은 안쪽 크기별·전체 화면의 초록 경계 상자. `quick` 은 가운데 픽셀과 종료 코드

## 알려진 것

- 셸은 PowerShell 을 씀. Claude Code 의 Bash 툴은 역슬래시 쌍과 한글 인자를 깨뜨림
- 창 캡처는 다른 창이 가리면 색·상자 크기가 틀리게 나옴. 캡처 전에 앱 창이 앞에 있는지 봄
- 로그 받기와 캡처를 한 PowerShell 프로세스에서 같이 하면 캡처 뒤 로그가 안 잡힌 적이 있음. 따로 돌림
- VS 에서 "모든 구성" 으로 속성을 고칠 때 조건 없는 그룹에 적히는지 구성마다 따로 적히는지 [미확인]. 빌드 결과는 같음
- 04편 DirectXMath 는 원고대로 GitHub 릴리스를 받거나, 같은 커밋(`jun2026` = `93e6399`)의 사본을 씀

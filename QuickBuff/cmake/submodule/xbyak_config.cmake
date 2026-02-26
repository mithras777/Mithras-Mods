#################################################
# Configure
#################################################
set(XBYAK_NAME      "xbyak")
set(XBYAK_REPO_URL  "https://github.com/herumi/xbyak")
set(XBYAK_REPO_TAG  "v7.30")
set(XBYAK_ROOT_DIR  "${PROJECT_EXTERN_ROOT_DIR}/${XBYAK_NAME}")
#################################################
# Fetch Project
#################################################
include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

if(EXISTS "${XBYAK_ROOT_DIR}/xbyak/xbyak.h")
	message(STATUS "Reusing existing ${XBYAK_NAME} at ${XBYAK_ROOT_DIR}")
else()
	FetchContent_Populate(
		${XBYAK_NAME}
		GIT_REPOSITORY  ${XBYAK_REPO_URL}
		GIT_TAG         ${XBYAK_REPO_TAG}
		SOURCE_DIR      ${XBYAK_ROOT_DIR}
	)
endif()

# Tie this to CommonLibSSE for SKSE_SUPPORT_XBYAK
target_include_directories(
	${COMMONLIBSSE_NAME}
	PUBLIC
		"${XBYAK_ROOT_DIR}"
)

target_compile_definitions(
	${COMMONLIBSSE_NAME} PUBLIC
		SKSE_SUPPORT_XBYAK=1
)

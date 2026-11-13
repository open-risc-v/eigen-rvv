# 上游接入：eigen（header-only），头文件拷进 <build>/pkg/prefix/include。

set(PKG_ROOT "${CMAKE_BINARY_DIR}/pkg")
if(NOT DEFINED PKG_PREFIX)
  set(PKG_PREFIX "${PKG_ROOT}/prefix")
  add_custom_target(pkg ALL
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cmake/pkg-build.sh" "${SRC_DIR}" "${PKG_ROOT}"
    COMMENT "安装上游 eigen 头文件（${SRC_DIR}）"
    VERBATIM)
  set(HAVE_PKG_TARGET TRUE)
endif()

add_executable(eigen_harness test/pkg_eigen.cpp)
if(HAVE_PKG_TARGET)
  add_dependencies(eigen_harness pkg)
endif()
target_include_directories(eigen_harness PRIVATE "${PKG_PREFIX}/include")
target_compile_options(eigen_harness PRIVATE ${RVV_FLAGS})
target_link_libraries(eigen_harness PRIVATE m)

add_test(NAME eigen_harness COMMAND eigen_harness)

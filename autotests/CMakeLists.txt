# SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(initinfotest initinfotest.cpp)
target_link_libraries(initinfotest PRIVATE Sonic::InitInfo Qt6::Test)
add_test(NAME initinfotest COMMAND initinfotest)

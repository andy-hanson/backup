#pragma once

#include <string>

enum TestMode { Test, Accept };

void test(const std::string& test_dir, const std::string& test_name, TestMode mode);

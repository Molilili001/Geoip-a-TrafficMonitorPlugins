// pch.h: 预编译头
// 仅编译一次以提升生成性能；若此处文件改动，将导致重新编译

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"

// 常用安全删除宏
#define SAFE_DELETE(p) do { \
    if ((p) != nullptr) { delete (p); (p) = nullptr; } \
} while (false)

#endif // PCH_H
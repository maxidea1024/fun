#pragma once

#include "fun/base/string_piece.h"
#include "fun/base/types.h"

String solveSudoku(const fun::StringPiece& puzzle);
const int kCells = 81;
extern const char kNoSolution[];

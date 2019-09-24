#pragma once

#include "fun/base/types.h"
#include "fun/base/string_piece.h"

String solveSudoku(const fun::StringPiece& puzzle);
const int kCells = 81;
extern const char kNoSolution[];

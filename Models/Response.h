#pragma once

enum class Response
{
    OK, // Продолжать ход.
    BACK, // Откат назад хода.
    REPLAY, // Играть заново.
    QUIT, // Выйти из игры.
    CELL // Ячейка.
};

#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
class Hand
{
  public:
    Hand(Board *board) : board(board)
    {
    }
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;
        // Изначально респонс ОК.
        Response resp = Response::OK;
        // Начальный ход.
        int x = -1, y = -1;
        // Смещение.
        int xc = -1, yc = -1;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    // Нажать на нужный участок доски.
                    x = windowEvent.motion.x;
                    y = windowEvent.motion.y;
                    // Смещение и ход.
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        // На один ход назад.
                        resp = Response::BACK;
                    }
                    else if (xc == -1 && yc == 8)
                    {
                        // Начать заново.
                        resp = Response::REPLAY;
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        // Поход на нужную ячейку.
                        resp = Response::CELL;
                    }
                    else
                    {
                        // Ничего не делать.
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        // Восстановить исходный размер окна.
                        board->reset_window_size();
                        break;
                    }
                }
                if (resp != Response::OK)
                    // Респонс был ОК.
                    break;
            }
        }
        return {resp, xc, yc};
    }

    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: {
                    // Если нажата кнопка мыши?
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        // Начать заново.
                        resp = Response::REPLAY;
                }
                break;
                }
                if (resp != Response::OK)
                    // Не респонс ОК.
                    break;
            }
        }
        // Возврат ответа запроса.
        return resp;
    }

  private:
    Board *board;
};

#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        // Время игры пошло.
        auto start = chrono::steady_clock::now();
        if (is_replay)
        {
            // Инициализация логики игры.
            logic = Logic(&board, &config);
            // Подсоединение настроек на лету.
            config.reload();
            // Прорисовка доски.
            board.redraw();
        }
        else
        {
            // Нарисовать доску.
            board.start_draw();
        }
        // Игра началась сначала.
        is_replay = false;

        // Кол-во исходных ходов на одну единицу меньше нуля.
        int turn_num = -1;
        // Не выходить из игры.
        bool is_quit = false;
        // Максимальное число ходов.
        const int Max_turns = config("Game", "MaxNumTurns");
        // Ход самой игры до макс. числа ходов.
        while (++turn_num < Max_turns)
        {
            // Серии битья шашек.
            beat_series = 0;
            // Четное или нечётное число ходов.
            logic.find_turns(turn_num % 2);
            if (logic.turns.empty())
                // При условии что не осталось ходов.
                break;
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Ходить белыми или чёрными шашками.
                auto resp = player_turn(turn_num % 2);
                if (resp == Response::QUIT)
                {
                    // Выйти из игры.
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    // Сыграть заново.
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // Назад на одну единицу хода.
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        // Откат доски.
                        board.rollback();
                        // Откат последнего хода.
                        --turn_num;
                    }
                    if (!beat_series)
                        // Откат последнего хода.
                        --turn_num;

                    // Откат доски.
                    board.rollback();
                    // Откат последнего хода.
                    --turn_num;
                    // Обнуление серии срубов.
                    beat_series = 0;
                }
            }
            else
                // Ходить ботом.
                bot_turn(turn_num % 2);
        }
        // Время закончилось.
        auto end = chrono::steady_clock::now();
        // Писать данные в лог файл.
        ofstream fout(project_path + "log.txt", ios_base::app);
        // Напечатать кол-во времени проведённого в игре.
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        // Закрыть поток файла.
        fout.close();

        // Если был запрос replay?
        if (is_replay)
            return play();
        // Если был запрос quit?
        if (is_quit)
            return 0;
        // Начальный результат = 2.
        int res = 2;
        // Если макс. кол-во ходов достигнуто - 0.
        if (turn_num == Max_turns)
        {
            res = 0;
        }
        // Если текущее число ходов чётно - 1.
        else if (turn_num % 2)
        {
            res = 1;
        }
        // Показать финал игры.
        board.show_final(res);
        // Подождать немного...
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            // Если ответ пришёл на запрос replay?
            is_replay = true;
            // Играть заново.
            return play();
        }
        // Возвращает результат очков игры.
        return res;
    }

  private:
    void bot_turn(const bool color)
    {
        // Время пошло.
        auto start = chrono::steady_clock::now();

        // Задержка игры ботом.
        auto delay_ms = config("Bot", "BotDelayMS");
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms);
        // Найти лучшие ходы ботом.
        auto turns = logic.find_best_turns(color);
        th.join();
        // Первый ход.
        bool is_first = true;
        // making moves
        for (auto turn : turns)
        {
            if (!is_first)
            {
                // Если ход не был первым.
                SDL_Delay(delay_ms);
            }
            // Ход перестаёт быть первым.
            is_first = false;
            // Увеличение серий битья шашек.
            beat_series += (turn.xb != -1);
            // Двигать шашку.
            board.move_piece(turn, beat_series);
        }

        // Время истекло.
        auto end = chrono::steady_clock::now();
        // Запись в лог.
        ofstream fout(project_path + "log.txt", ios_base::app);
        // Запись данных о выполнении ботом.
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        // Закрыть файловый поток.
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // return 1 if quit
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        // Засветить ближайшие ячейки.
        board.highlight_cells(cells);
        // Позиции для перемещения шашки.
        move_pos pos = {-1, -1, -1, -1};
        // Переместить шашку.
        POS_T x = -1, y = -1;
        // trying to make first move
        while (true)
        {
            // Получить выделенную ячейку.
            auto resp = hand.get_cell();
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp);
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

            // Пока не выбрана ячейка.
            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    // Корректный ход.
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second})
                {
                    // Ходить на выбранную ячейку.
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)
                break;
            if (!is_correct)
            {
                // Если некорректно выбрана ячейка.
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                // Возврат хода.
                x = -1;
                y = -1;
                // Продолжение выполнения программы.
                continue;
            }
            // Роспись хода.
            x = cell.first;
            y = cell.second;
            // Очистка выделения.
            board.clear_highlight();
            // Установить активной текущую ячейку.
            board.set_active(x, y);
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            // Засветить ячейки.
            board.highlight_cells(cells2);
        }
        // Очисьить выделение.
        board.clear_highlight();
        // Очистить активную ячейку.
        board.clear_active();
        // Переместить шашку.
        board.move_piece(pos, pos.xb != -1);
        if (pos.xb == -1)
            // Продолжать ход.
            return Response::OK;
        // continue beating while can
        beat_series = 1;
        while (true)
        {
            // Найти ходы шашек.
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats)
                // Не осталось ходов.
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            // Засветить ячейки.
            board.highlight_cells(cells);
            // Установить активную ячейку.
            board.set_active(pos.x2, pos.y2);
            // trying to make move
            while (true)
            {
                // Получить ячейку.
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)
                    // Не получен ответ cell.
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

                // Изначально не сделан ход.
                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        // Ход правильный.
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct)
                    // Ход не сделан.
                    continue;

                // Убрать выделение.
                board.clear_highlight();
                // Убрать активную ячейку.
                board.clear_active();
                // Увеличить серию битья.
                beat_series += 1;
                // Двигать шашку.
                board.move_piece(pos, beat_series);
                // Выход из этого цикла.
                break;
            }
        }

        // Выполнен ход успешно.
        return Response::OK;
    }

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};

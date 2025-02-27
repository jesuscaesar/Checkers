#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        // Перезагружает настройки выполнения программы.
        reload();
    }

    void reload()
    {
        // Берёт настройки программы из файла.
        std::ifstream fin(project_path + "settings.json");
        // Записывает данные из файла в переменную config.
        fin >> config;
        // Закрывает поток файлов.
        fin.close();
    }

    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        // Возвращает данные из настроек программы.
        return config[setting_dir][setting_name];
    }

  private:
      // Создаёт переменную типа данных JSON config.
      json config;
};

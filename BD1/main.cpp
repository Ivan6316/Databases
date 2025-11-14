#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <string>

// Структура для хранения данных клиента
struct ClientInfo {
    int id{};
    std::string first_name{};
    std::string last_name{};
    std::string email{};
    std::vector<std::string> phones{};
};

class CustomerDatabase
{
private:
    pqxx::connection _database_;

public:
    // Конструктор
    CustomerDatabase() : _database_(
        "host=localhost "
        "port=5432 "
        "dbname=my_bd "
        "user=postgres "
        "password=228328528")
    {
        if (!_database_.is_open())
        {
            throw std::runtime_error("Ошибка подключения базы данных!");
        }
        std::cout << "База данных подключена!" << std::endl;
        std::cout << "Имя базы данных: " << _database_.dbname() << std::endl;
    }

    // Создание структуры БД
    void creatingAStructure()
    {
        pqxx::work db{ _database_ };

        // Создание таблицы с основной информации о пользователе
        db.exec(
            "CREATE TABLE IF NOT EXISTS clients (id SERIAL PRIMARY KEY, first_name VARCHAR(50) NOT NULL, "
            "last_name VARCHAR(50) NOT NULL, email VARCHAR(100));"
        );

        // Создание таблицы с телефонными номерами пользователей
        db.exec(
            "CREATE TABLE IF NOT EXISTS client_phones (id SERIAL PRIMARY KEY, "
            "client_id INTEGER NOT NULL REFERENCES clients(id) ON DELETE CASCADE,"
            "phone_number VARCHAR(20) NOT NULL);"
        );

        db.commit();
    }

    // Добавление нового клиента - возвращает ID созданного клиента
    int addingNewClient(const std::string& first_name, const std::string& last_name,
        const std::string& email, const std::vector<std::string>& phone_numbers = {})
    {
        pqxx::work db{ _database_ };

        // Добавление к основной таблице
        db.exec(
            "INSERT INTO clients (first_name, last_name, email) VALUES("
            + db.quote(first_name) + ", " + db.quote(last_name)
            + ", " + db.quote(email) + ");"
        );

        // Получаем ID только что добавленного клиента
        pqxx::result result = db.exec("SELECT LASTVAL()");
        int client_id{ result[0][0].as<int>() };

        // Добавляем телефоны для этого клиента
        for (const auto& phone : phone_numbers)
        {
            db.exec(
                "INSERT INTO client_phones (client_id, phone_number) VALUES("
                + db.quote(client_id) + ", " + db.quote(phone) + ");"
            );
        }

        db.commit();
        return client_id;
    }

    // Добавление телефона для клиента
    void addingPhoneNumber(int clientId, const std::string& new_number)
    {
        pqxx::work db{ _database_ };

        // Проверка существует ли пользователь
        pqxx::result clientCheck = db.exec(
            "SELECT id FROM clients WHERE id = " + db.quote(clientId) + ";"
        );

        if (clientCheck.empty())
        {
            throw std::runtime_error("Данного пользователя не существует!");
        }

        // Добавление телефона
        db.exec(
            "INSERT INTO client_phones (client_id, phone_number) VALUES("
            + db.quote(clientId) + ", " + db.quote(new_number) + ");"
        );

        db.commit();
    }

    // Изменение данных о клиенте
    void changingData(int clientId, const std::string& newFirstName,
        const std::string& newLastName, const std::string& newEmail)
    {
        pqxx::work db{ _database_ };

        // Проверка существует ли пользователь
        pqxx::result clientCheck = db.exec(
            "SELECT id FROM clients WHERE id = " + db.quote(clientId) + ";"
        );

        if (clientCheck.empty())
        {
            throw std::runtime_error("Данного пользователя не существует!");
        }

        // Изменение данных
        db.exec(
            "UPDATE clients SET first_name = " + db.quote(newFirstName)
            + ", last_name = " + db.quote(newLastName) + ", email = "
            + db.quote(newEmail) + " WHERE id = " + std::to_string(clientId) + ";"
        );

        db.commit();
    }

    // Удаление телефона у клиента
    void deletingPhoneNumber(int clientId, const std::string& phone_number)
    {
        pqxx::work db{ _database_ };

        // Проверка существует ли пользователь
        pqxx::result clientCheck = db.exec(
            "SELECT id FROM clients WHERE id = " + db.quote(clientId) + ";"
        );

        if (clientCheck.empty())
        {
            throw std::runtime_error("Данного пользователя не существует!");
        }

        // Удаление телефона
        db.exec(
            "DELETE FROM client_phones WHERE client_id = " + std::to_string(clientId)
            + " AND phone_number = " + db.quote(phone_number) + ";"
        );

        db.commit();
    }

    // Удаление клиента
    void deletingClient(int clientId)
    {
        pqxx::work db{ _database_ };

        // Проверка существует ли пользователь
        pqxx::result clientCheck = db.exec(
            "SELECT id FROM clients WHERE id = " + db.quote(clientId) + ";"
        );

        if (clientCheck.empty())
        {
            throw std::runtime_error("Данного пользователя не существует!");
        }

        // Удаление пользователя (CASCADE автоматически удалит телефоны)
        db.exec("DELETE FROM clients WHERE id = " + std::to_string(clientId) + ";");
        db.commit();
    }

    // Поиск клиента
    std::vector<ClientInfo> findClient(const std::string& search_term)
    {
        pqxx::work db{ _database_ };
        std::vector<ClientInfo> results;

        // Ищем клиента по всем полям
        std::string query =
            "SELECT DISTINCT c.id, c.first_name, c.last_name, c.email, "
            "p.phone_number as phone "
            "FROM clients c "
            "LEFT JOIN client_phones p ON c.id = p.client_id "
            "WHERE c.first_name ILIKE " + db.quote("%" + search_term + "%") + " "
            "OR c.last_name ILIKE " + db.quote("%" + search_term + "%") + " "
            "OR c.email ILIKE " + db.quote("%" + search_term + "%") + " "
            "OR p.phone_number ILIKE " + db.quote("%" + search_term + "%") + " "
            "ORDER BY c.id;";

        pqxx::result db_result = db.exec(query);

        // Обрабатываем результаты
        int current_id{ -1 };
        ClientInfo current_client;

        for (const auto& row : db_result) {
            int client_id = row["id"].as<int>();

            if (client_id != current_id)
            {
                // Сохраняем предыдущего клиента
                if (current_id != -1)
                {
                    results.push_back(current_client);
                }

                // Начинаем нового клиента
                current_id = client_id;
                current_client = ClientInfo{
                    client_id,
                    row["first_name"].as<std::string>(),
                    row["last_name"].as<std::string>(),
                    row["email"].as<std::string>(),
                    std::vector<std::string>()
                };
            }

            // Добавляем телефон
            if (!row["phone"].is_null())
            {
                current_client.phones.push_back(row["phone"].as<std::string>());
            }
        }

        // Добавляем последнего клиента
        if (current_id != -1)
        {
            results.push_back(current_client);
        }

        return results;
    }
};

// Вспомогательная функция для красивого вывода результатов поиска
void printSearchResults(const std::vector<ClientInfo>& clients, const std::string& search_term)
{
    if (clients.empty()) {
        std::cout << "Клиенты по запросу '" << search_term << "' не найдены." << std::endl;
        return;
    }

    std::cout << "Найдено клиентов: " << clients.size() << std::endl;
    std::cout << "=========================================" << std::endl;

    for (const auto& client : clients) {
        std::cout << "ID: " << client.id << std::endl;
        std::cout << "Имя: " << client.first_name << std::endl;
        std::cout << "Фамилия: " << client.last_name << std::endl;
        std::cout << "Email: " << client.email << std::endl;
        std::cout << "Телефоны: ";
        if (client.phones.empty()) {
            std::cout << "нет телефонов";
        }
        else {
            for (size_t i = 0; i < client.phones.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << client.phones[i];
            }
        }
        std::cout << std::endl;
        std::cout << "-----------------------------------------" << std::endl;
    }
}

int main()
{
    // Подключение Русского языка
    setlocale(LC_ALL, "rus");

    try {
        CustomerDatabase database;

        // Создание структуры
        database.creatingAStructure();

        // Добавление нового пользователя
        std::vector<std::string> numberPhone{ "89029233232", "89930231143" };
        int newClientId = database.addingNewClient("Ivan", "Belousov", "vanebelousov@", numberPhone);
        std::cout << "Добавлен новый клиент с ID: " << newClientId << std::endl;

        // Добавление нового номера телефона
        database.addingPhoneNumber(1, "89000344664");
        std::cout << "Добавлен новый номер телефона" << std::endl;

        // Изменение данных
        database.changingData(1, "Katy", "Ivanova", "kate323@");
        std::cout << "Данные клиента обновлены" << std::endl;

        // Удаление телефона
        database.deletingPhoneNumber(1, "89029233232");
        std::cout << "Номер телефона удален" << std::endl;

        // Поиск клиентов и работа с результатами
        auto foundClients{ database.findClient("Katy") };
        printSearchResults(foundClients, "Katy");

        // Удаление клиента
        database.deletingClient(1);
        std::cout << "Клиент удален" << std::endl;

    }
    catch (const pqxx::sql_error& e)
    {
        std::cerr << "Ошибка SQL: " << e.what() << std::endl;
        std::cerr << "Запрос: " << e.query() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
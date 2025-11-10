#include <iostream>
#include <pqxx/pqxx>
#include <vector>

class CustomerDatabase
{
private:
	pqxx::connection _database_;

public:
    // Процесс подключение к БД
    CustomerDatabase() : _database_(
        "host=localhost "
        "port=5432 "
        "dbname=my_bd "
        "user=postgres "
        "password=228328528")
    {
        try
        {
            // Проверка на подключение
            if (_database_.is_open())
            {
                std::cout << "База данных подключена!" << std::endl;
                std::cout << "Имя базы данных: " << _database_.dbname() << std::endl;
            }
            else
            {
                std::cout << "Ошибка подключения базы данных!" << std::endl;
            }
        }
        catch (const pqxx::sql_error& e)
        {
            std::cout << "Ошибка SQL: " << e.what() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "Общая ошибка: " << e.what() << std::endl;
        }
    }

    // Создание структуры БД
    void creatingAStructure()
    {
        pqxx::work db{ _database_ };

        try
        {
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
        catch (const pqxx::sql_error& e)
        {
            std::cout << "Ошибка SQL: " << e.what() << std::endl;
            std::cout << "Запрос: " << e.query() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "Общая ошибка: " << e.what() << std::endl;
        }
    }

    // Добавление нового клиента
    void addingNewClient(std::string first_name, std::string last_name, std::string email,
        const std::vector<std::string>& phone_number)
    {
        pqxx::work db{ _database_ };

        try
        {
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
            for (const auto& phone : phone_number)
            {
                db.exec(
                    "INSERT INTO client_phones (client_id, phone_number) VALUES("
                    + db.quote(client_id) + ", " + db.quote(phone) + ");"
                );
            }

            db.commit();
        }
        catch (const pqxx::sql_error& e)
        {
            std::cout << "Ошибка SQL: " << e.what() << std::endl;
            std::cout << "Запрос: " << e.query() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "Общая ошибка: " << e.what() << std::endl;
        }
    }

    // Добавление телефона для клиента
    void addingPhoneNumber(int clientId, std::string new_number)
    {
        pqxx::work db{ _database_ };

        try
        {
            // Проверка существует ли пользователь
            pqxx::result clientCheck = db.exec(
                "SELECT id FROM clients WHERE id = " + db.quote(clientId) + ";"
            );

            if (clientCheck.empty())
            {
                std::cout << "Данного пользователя не существует!" << std::endl;
                return;
            }

            // Добавление телефона
            db.exec(
                "INSERT INTO client_phones (client_id, phone_number) VALUES("
                + db.quote(clientId) + ", " + db.quote(new_number) + ");"
            );

            db.commit();
        }
        catch (const pqxx::sql_error& e)
        {
            std::cout << "Ошибка SQL:" << e.what() << std::endl;
            std::cout << "Запрос: " << e.query() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "Общая ошибка: " << e.what() << std::endl;
        }
    }

    // Изменение данных о клиенте
    void changingData(int clientId, std::string newFirstName, std::string newLastName,
        std::string newEmail)
    {
        pqxx::work db{ _database_ };

        try
        {
            // Проверка существует ли пользователь
            pqxx::result clientCheck = db.exec(
                "SELECT id FROM clients WHERE id = " + db.quote(clientId) + ";"
            );

            if (clientCheck.empty())
            {
                std::cout << "Данного пользователя не существует!" << std::endl;
                return;
            }

            // Изменение данных
            db.exec(
                "UPDATE clients SET first_name = " + db.quote(newFirstName)
                + ", last_name = " + db.quote(newLastName) + ", email = "
                + db.quote(newEmail) + " WHERE id = " + std::to_string(clientId) + ";"
            );

            db.commit();
        }
        catch (const pqxx::sql_error& e)
        {
            std::cout << "Ошибка SQL: " << e.what() << std::endl;
            std::cout << "Запрос: " << e.query() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "Общая ошибка: " << e.what() << std::endl;
        }
    }

    // Удаление телефона у клиента
    void deletingPhoneNumber(int clientId, std::string phone_number)
    {
        pqxx::work db{ _database_ };

        try
        {
            // Проверка существует ли пользователь
            pqxx::result clientCheck = db.exec(
                "SELECT id FROM clients WHERE id = " + db.quote(clientId) + ";"
            );

            if (clientCheck.empty())
            {
                std::cout << "Данного пользователя не существует!" << std::endl;
                return;
            }

            // Удаление телефона
            db.exec(
                "DELETE FROM client_phones WHERE client_id = " + std::to_string(clientId)
                + " AND phone_number = " + db.quote(phone_number) + ";"
            );

            db.commit();
        }
        catch (const pqxx::sql_error& e)
        {
            std::cout << "Ошибка SQL: " << e.what() << std::endl;
            std::cout << "Запрос: " << e.query() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "Общая ошибка: " << e.what() << std::endl;
        }
    }

    // Удаление клиента
    void deletingClient(int clientId)
    {
        pqxx::work db{ _database_ };

        try
        {
            // Проверка существует ли пользователь
            pqxx::result clientCheck = db.exec(
                "SELECT id FROM clients WHERE id = " + db.quote(clientId) + ";"
            );

            if (clientCheck.empty())
            {
                std::cout << "Данного пользователя не существует!" << std::endl;
                return;
            }

            // Удаление пользователя
            db.exec(
                "DELETE FROM client_phones WHERE client_id = " + std::to_string(clientId)
                + ";"
            );

            db.exec(
                "DELETE FROM clients WHERE id = " + std::to_string(clientId) + ";"
            );

            db.commit();
        }
        catch (const pqxx::sql_error& e)
        {
            std::cout << "Ошибка SQL: " << e.what() << std::endl;
            std::cout << "Запрос: " << e.query() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "Общая ошибка: " << e.what() << std::endl;
        }
    }

    // Поиск клиента
    void findClient(const std::string& search_term)
    {
        pqxx::work db{ _database_ };

        try
        {
            // Ищем клиента по всем полям
            std::string query =
                "SELECT DISTINCT c.id, c.first_name, c.last_name, c.email, "
                "COALESCE(string_agg(p.phone_number, ', '), 'no phones') as phones "
                "FROM clients c "
                "LEFT JOIN client_phones p ON c.id = p.client_id "
                "WHERE c.first_name ILIKE " + db.quote("%" + search_term + "%") + " "
                "OR c.last_name ILIKE " + db.quote("%" + search_term + "%") + " "
                "OR c.email ILIKE " + db.quote("%" + search_term + "%") + " "
                "OR p.phone_number ILIKE " + db.quote("%" + search_term + "%") + " "
                "GROUP BY c.id, c.first_name, c.last_name, c.email "
                "ORDER BY c.id;";

            pqxx::result result = db.exec(query);

            if (result.empty()) {
                std::cout << "Клиенты по запросу '" << search_term << "' не найдены." << std::endl;
                return;
            }

            std::cout << "Найдено клиентов: " << result.size() << std::endl;
            std::cout << "=========================================" << std::endl;

            for (const auto& row : result) {
                std::cout << "ID: " << row["id"].as<int>() << std::endl;
                std::cout << "Имя: " << row["first_name"].as<std::string>() << std::endl;
                std::cout << "Фамилия: " << row["last_name"].as<std::string>() << std::endl;
                std::cout << "Email: " << row["email"].as<std::string>() << std::endl;
                std::cout << "Телефоны: " << row["phones"].as<std::string>() << std::endl;
                std::cout << "-----------------------------------------" << std::endl;
            }

        }
        catch (const pqxx::sql_error& e)
        {
            std::cout << "Ошибка SQL: " << e.what() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "Общая ошибка: " << e.what() << std::endl;
        }
    }
};

int main()
{
	// Подключение Русского языка
    setlocale(LC_ALL, "rus");

	CustomerDatabase database;

    // Создание структуры
    database.creatingAStructure();

    // Добавление нового пользователя
    std::vector<std::string> numberPhone{ "89029233232", "89930231143" };
    database.addingNewClient("Ivan", "Belousov", "vanebelousov@", numberPhone);

    // Добавление нового номера телефона
    database.addingPhoneNumber(1, "89000344664");

    // Изменение данных
    database.changingData(3, "Katy", "Ivanova", "kate323@");

    // Удаление телефона
    database.deletingPhoneNumber(1, "89029233232");

    // Удаление клиента
    database.deletingClient(1);

    // Поиск клиентов
    database.findClient("Katy");


	return EXIT_SUCCESS;
}
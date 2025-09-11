#include <iostream>
#include <string>
#include <cstdlib>
#include <mysql.h>
#include <sstream>
#include <map>
using namespace std;

// DB config
const char* DB_HOST = "db";
const char* DB_USER = "root";
const char* DB_PASS = "yourpassword";
const char* DB_NAME = "student_mgmt";

// URL decoding (basic)
string url_decode(const string& str) {
    string decoded;
    char ch;
    int i, ii;
    for (i = 0; i < str.length(); i++) {
        if (int(str[i]) == 37) {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            decoded += ch;
            i = i + 2;
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

// Parse form data
void parse_post_data(string& data, map<string, string>& fields) {
    size_t pos = 0;
    while ((pos = data.find("&")) != string::npos) {
        string token = data.substr(0, pos);
        size_t eq = token.find("=");
        if (eq != string::npos) {
            string key = url_decode(token.substr(0, eq));
            string value = url_decode(token.substr(eq + 1));
            fields[key] = value;
        }
        data.erase(0, pos + 1);
    }
    if (!data.empty()) {
        size_t eq = data.find("=");
        if (eq != string::npos) {
            string key = url_decode(data.substr(0, eq));
            string value = url_decode(data.substr(eq + 1));
            fields[key] = value;
        }
    }
}

// Escape string for SQL
string escape(MYSQL* conn, const string& str) {
    char* escaped = new char[str.length() * 2 + 1];
    mysql_real_escape_string(conn, escaped, str.c_str(), str.length());
    string result(escaped);
    delete[] escaped;
    return result;
}

// Ensure required tables exist
void ensure_tables_exist(MYSQL* conn) {
    const char* create_users_table = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            email VARCHAR(100) NOT NULL UNIQUE,
            password VARCHAR(255) NOT NULL,
            role ENUM('student','faculty') NOT NULL
        )
    )";

    const char* create_lost_found_table = R"(
        CREATE TABLE IF NOT EXISTS lost_found_items (
            id INT AUTO_INCREMENT PRIMARY KEY,
            item_name VARCHAR(255) NOT NULL,
            description TEXT NOT NULL,
            status ENUM('lost', 'found') NOT NULL,
            reported_by INT NOT NULL,
            reported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (reported_by) REFERENCES users(id) ON DELETE CASCADE
        )
    )";

    if (mysql_query(conn, create_users_table) != 0) {
        cerr << "{\"status\":\"error\",\"message\":\"Failed to create 'users' table: " << mysql_error(conn) << "\"}";
        exit(1);
    }

    if (mysql_query(conn, create_lost_found_table) != 0) {
        cerr << "{\"status\":\"error\",\"message\":\"Failed to create 'lost_found_items' table: " << mysql_error(conn) << "\"}";
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    cout << "Content-Type: application/json\n\n";

    // CLI mode: register from command line
    if (argc == 6 && string(argv[1]) == "register") {
        MYSQL* conn = mysql_init(NULL);
        if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 3306, NULL, 0)) {
            cout << R"({"status":"error","message":"DB connection failed"})";
            return 1;
        }

        ensure_tables_exist(conn);

        string name = argv[2];
        string email = argv[3];
        string password = argv[4];
        string role = argv[5];

        string query = "INSERT INTO users (name, email, password, role) VALUES ('" +
            escape(conn, name) + "','" + escape(conn, email) + "','" +
            escape(conn, password) + "','" + escape(conn, role) + "')";

        if (mysql_query(conn, query.c_str()) != 0) {
            cout << R"({"status":"error","message":"Email already exists or insert failed"})";
        } else {
            cout << R"({"status":"success","message":"Registered successfully via CLI"})";
        }

        mysql_close(conn);
        return 0;
    }

    // CGI mode: form submissions
    string post_data;
    char* len_str = getenv("CONTENT_LENGTH");
    if (!len_str) {
        cout << R"({"status":"error","message":"No content length"})";
        return 1;
    }

    int len = atoi(len_str);
    for (int i = 0; i < len; ++i) {
        char c = getchar();
        post_data += c;
    }

    map<string, string> fields;
    parse_post_data(post_data, fields);

    string action = fields["action"];
    string name = fields["name"];
    string email = fields["email"];
    string password = fields["password"];
    string role = fields["role"];

    MYSQL* conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 3306, NULL, 0)) {
        cout << R"({"status":"error","message":"DB connection failed"})";
        return 1;
    }

    ensure_tables_exist(conn);

    if (action == "register") {
        string query = "INSERT INTO users (name, email, password, role) VALUES ('" +
            escape(conn, name) + "','" + escape(conn, email) + "','" +
            escape(conn, password) + "','" + escape(conn, role) + "')";

        if (mysql_query(conn, query.c_str()) != 0) {
            cout << R"({"status":"error","message":"Email already exists or insert failed"})";
        } else {
            cout << R"({"status":"success","message":"Registered successfully"})";
        }
    }
    else if (action == "login") {
        string query = "SELECT id, name, role FROM users WHERE email='" +
            escape(conn, email) + "' AND password='" + escape(conn, password) + "'";

        if (mysql_query(conn, query.c_str()) != 0) {
            cout << R"({"status":"error","message":"Query failed"})";
            mysql_close(conn);
            return 1;
        }

        MYSQL_RES* result = mysql_store_result(conn);
        if (mysql_num_rows(result) == 0) {
            cout << R"({"status":"error","message":"Invalid email or password"})";
        } else {
            MYSQL_ROW row = mysql_fetch_row(result);
            cout << "{";
            cout << "\"status\":\"success\",";
            cout << "\"message\":\"Login successful\",";
            cout << "\"user_id\":" << row[0] << ",";
            cout << "\"name\":\"" << row[1] << "\",";
            cout << "\"role\":\"" << row[2] << "\"";
            cout << "}";
        }

        mysql_free_result(result);
    }
    else {
        cout << R"({"status":"error","message":"Invalid action"})";
    }

    mysql_close(conn);
    return 0;
}
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
const char* DB_PASS = "yourpassword";  // Update if different
const char* DB_NAME = "student_mgmt";

// URL decode
string url_decode(const string& str) {
    string decoded;
    char ch;
    int i, ii;
    for (i = 0; i < str.length(); i++) {
        if (int(str[i]) == 37) {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            decoded += ch;
            i += 2;
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

// Parse POST data
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

// SQL escape helper
string escape(MYSQL* conn, const string& str) {
    char* escaped = new char[str.length() * 2 + 1];
    mysql_real_escape_string(conn, escaped, str.c_str(), str.length());
    string result(escaped);
    delete[] escaped;
    return result;
}

int main() {
    cout << "Content-Type: application/json\n\n";

    // Get content length
    char* len_str = getenv("CONTENT_LENGTH");
    if (!len_str) {
        cout << R"({"status":"error","message":"Missing content length"})";
        return 1;
    }

    int len = atoi(len_str);
    string post_data;
    for (int i = 0; i < len; ++i) {
        char c = getchar();
        post_data += c;
    }

    map<string, string> fields;
    parse_post_data(post_data, fields);

    string item_name = fields["item_name"];
    string description = fields["description"];
    string status = fields["status"];
    string reported_by = fields["reported_by"];

    // Basic validation
    if (item_name.empty() || description.empty() || status.empty() || reported_by.empty()) {
        cout << R"({"status":"error","message":"Missing required fields"})";
        return 1;
    }

    MYSQL* conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, 3306, NULL, 0)) {
        cout << R"({"status":"error","message":"Database connection failed"})";
        return 1;
    }

    // Create table if not exists
    const char* create_table_query = R"(
        CREATE TABLE IF NOT EXISTS lost_found_items (
            id INT AUTO_INCREMENT PRIMARY KEY,
            item_name VARCHAR(255) NOT NULL,
            description TEXT,
            status ENUM('lost', 'found') NOT NULL,
            reported_by INT,
            reported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    if (mysql_query(conn, create_table_query) != 0) {
        cout << R"({"status":"error","message":"Failed to create table"})";
        mysql_close(conn);
        return 1;
    }

    // Insert record
    string query = "INSERT INTO lost_found_items (item_name, description, status, reported_by) VALUES ('" +
        escape(conn, item_name) + "','" + escape(conn, description) + "','" +
        escape(conn, status) + "'," + escape(conn, reported_by) + ")";

    if (mysql_query(conn, query.c_str()) != 0) {
        cout << R"({"status":"error","message":"Insert failed","details":")" << mysql_error(conn) << R"("})";
    } else {
        cout << R"({"status":"success","message":"Lost/Found item reported successfully"})";
    }

    mysql_close(conn);
    return 0;
}
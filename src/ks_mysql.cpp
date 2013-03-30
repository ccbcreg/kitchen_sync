#include "endpoint.h"

#include <stdexcept>
#include <mysql.h>

#define MYSQL_5_6_5 50605

class MySQLClient {
public:
	MySQLClient(
		const char *database_host,
		const char *database_port,
		const char *database_name,
		const char *database_username,
		const char *database_password,
		bool readonly);
	~MySQLClient();

protected:
	void execute(const char *sql);
	void start_transaction(bool readonly);

private:
	MYSQL mysql;
};

MySQLClient::MySQLClient(
	const char *database_host,
	const char *database_port,
	const char *database_name,
	const char *database_username,
	const char *database_password,
	bool readonly) {

	// mysql_real_connect takes separate params for numeric ports and unix domain sockets
	int port = 0;
	const char *socket = NULL;
	if (database_port) {
		if (*database_port >= '0' && *database_port <= '9') {
			port = atoi(database_port);
		} else {
			socket = database_port;
		}
	}

	mysql_init(&mysql);
	mysql_options(&mysql, MYSQL_READ_DEFAULT_GROUP, "ks_mysql");
	if (!mysql_real_connect(&mysql, database_host, database_username, database_password, database_name, port, socket, 0)) {
		throw runtime_error(mysql_error(&mysql));
	}

	// although we start the transaction here, in reality mysql's system catalogs are non-transactional
	// and do not give a consistent snapshot
	start_transaction(readonly);
}

MySQLClient::~MySQLClient() {
	mysql_close(&mysql);
}

void MySQLClient::execute(const char *sql) {
	if (mysql_query(&mysql, "BEGIN")) {
		throw runtime_error(mysql_error(&mysql));
	}
}

void MySQLClient::start_transaction(bool readonly) {
	execute("SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
	execute(readonly && mysql_get_server_version(&mysql) >= MYSQL_5_6_5 ? "START TRANSACTION READ ONLY" : "START TRANSACTION");
}

int main(int argc, char *argv[]) {
	return endpoint_main<MySQLClient>(argc, argv);
}

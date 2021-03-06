#include <QCoreApplication>
#include <QSettings>
#include <QFileInfo>

#include "cwmpInform.h"
#include "cwmpServer.h"
#include <bson.h>
#include <mongoc.h>
//curl http://192.168.5.17:7547/b22537dbdf4291cb4c7b0152ca139f69 init connection request

int
mongo_insert_inform(char *SerialNumber, char *Time, char *OpState,
        char *ConnectionRequestURL, char *HNBName, char *NumOfActiveUE) {
    const char *uri_string = "mongodb://localhost:27017";
    mongoc_uri_t *uri;
    mongoc_client_t *client;
    mongoc_database_t *database;
    mongoc_collection_t *collection;
    bson_t *command, reply, *update, *query;
    bson_error_t error;
    char *str;
    bool retval;

    /*
     * Required to initialize libmongoc's internals
     */
    mongoc_init();

    /* Optionally get MongoDB URI from command line
     */

    /*
     * Safely create a MongoDB URI object from the given string
     */
    uri = mongoc_uri_new(uri_string);
    if (!uri) {
        fprintf(stderr,
                "failed to parse URI: %s\n"
                "error message:       %s\n",
                uri_string,
                error.message);
        return EXIT_FAILURE;
    }

    /*
     * Create a new client instance
     */
    client = mongoc_client_new_from_uri(uri);
    if (!client) {
        return EXIT_FAILURE;
    }

    /*
     * Register the application name so we can track it in the profile logs
     * on the server. This can also be done from the URI (see other examples).
     */

    /*
     * Get a handle on the database "db_name" and collection "coll_name"
     */
    database = mongoc_client_get_database(client, "tr069");
    collection = mongoc_client_get_collection(client, "tr069", "inform");

    /*
     * Do work. This example pings the database, prints the result as JSON and
     * performs an insert
     */
    command = BCON_NEW("ping", BCON_INT32(1));

    retval = mongoc_client_command_simple(
            client, "admin", command, NULL, &reply, &error);

    if (!retval) {
        fprintf(stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }

    str = bson_as_json(&reply, NULL);
    printf("%s\n", str);

    query = BCON_NEW("SerialNumber", SerialNumber);

    update = BCON_NEW("$set", "{",
            "SerialNumber", SerialNumber,
            "TIME", Time,
            "OpState", OpState,
            "ConnectionRequestURL", ConnectionRequestURL,
            "HNBName", HNBName,
            "NumOfActiveUE", NumOfActiveUE,
            "}");
    if (!mongoc_collection_update(collection, MONGOC_UPDATE_UPSERT, query, update, NULL, &error)) {
        fprintf(stderr, "%s\n", error.message);
    }

    bson_destroy(update);
    bson_destroy(query);
    bson_destroy(&reply);
    bson_destroy(command);
    bson_free(str);

    /*
     * Release our handles and clean up libmongoc
     */
    mongoc_collection_destroy(collection);
    mongoc_database_destroy(database);
    mongoc_uri_destroy(uri);
    mongoc_client_destroy(client);
    mongoc_cleanup();

    return EXIT_SUCCESS;
}

int port = 0;
QString interface;

int parseConfig(QString file) {
    QSettings settings(file,
            QSettings::IniFormat);

    settings.beginGroup("MAIN");
    const QStringList childKeys = settings.childKeys();

    foreach(const QString &childKey, childKeys) {
        if (childKey == "port") {
            port = settings.value(childKey).toInt();
        } else if (childKey == "bind") {
            interface = settings.value(childKey).toString();
        } else {
            QString var = settings.value(childKey).toString();
            qDebug() << childKey << ":" << var;
        }
    }
    settings.endGroup();
    return 0;
}

int main(int argc, char *argv[]) {
    QString configFile = "cwmpd.ini";
    QFile path(configFile);
    bool fileExists = QFileInfo(path).exists() &&
            QFileInfo(path).isFile();
    if (fileExists)
        parseConfig(configFile);
    else {
        qDebug() << "Config file " << configFile << "not found!";
        return (-1);
    }
    QCoreApplication app(argc, argv);
    CWMPServer server;
    if (port == 0)
        port = 8080;
    server.listen(QHostAddress::Any, port);
    qDebug() << "Start TCP Server" << interface << ":" << port;

    app.exec();

    return 0;
}


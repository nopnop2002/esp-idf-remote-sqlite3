# esp-idf-remote-sqlite3
Remote sqlite3 access example for esp-idf.   
You can access sqlite3 on the server over the network.   
Use [this](https://github.com/alixaxel/ArrestDB) as PHP script of WEB server.   

![remote-sqlite3](https://user-images.githubusercontent.com/6020549/97775795-c21af600-1ba6-11eb-9d02-04dcaca058c7.jpg)

# Server Side

## Install sqlite3   
```
$ sudo apt install sqlite3
```

## Create sqlite3 database   
Run the following script in any directory.   
```
#!/bin/bash
#set -x

if [ -z $(which sqlite3) ];then
  echo "sqlite3 not found."
  exit -1
fi

cd $(dirname $0)
dbfile=$1
if [ $# -ne 1 ];then
  dbfile="example.db"
  echo "Create ${dbfile}"
fi


# Create Database
if [ ! -e ${dbfile} ];then
  echo ".open ${dbfile}" | sqlite3
  echo "build database"
else
  echo "database already exists"
fi

option="-noheader -separator ,"
sqlite="sqlite3 ${option} ${dbfile} "

# Create Table
${sqlite} "create table if not exists customers( \
id integer primary key autoincrement, \
name varchar(255), \
gender int);"

# Insert Data
${sqlite} "insert into customers(name,gender) values(\"Luis\", 1);"
${sqlite} "insert into customers(name,gender) values(\"Leonie\", 1);"
${sqlite} "insert into customers(name,gender) values(\"Francois\", 2);"
${sqlite} "insert into customers(name,gender) values(\"Bjorn\", 2);"

${sqlite} "select * from customers;"
```

## Install ArrestDB
```
$ cd $HOME
$ git clone https://github.com/alixaxel/ArrestDB
$ cd ArrestDB
$ vi index.php

Set the full path of the database in the following line.

$dsn = '';

For example, if the database is /home/sqlite/example.db, it will be as follows.

$dsn = 'sqlite:///home/sqlite/example.db';

```

## Install PHP
```
$ cd $HOME
$ sudo apt install php

$ php --version
$ php --version
PHP 8.2.29 (cli) (built: Jul  3 2025 16:16:05) (NTS)
Copyright (c) The PHP Group
Zend Engine v4.2.29, Copyright (c) Zend Technologies
    with Zend OPcache v8.2.29, Copyright (c), by Zend Technologies
```

## Install PDO driver for Sqlite3
```
$ sudo apt install php[Version]-sqlite3
```

When the PHP version is 8.2.x, execute the following.   
```
$ sudo apt install php8.2-sqlite3
```


## Start the PHP built-in web server
```
$ php -S 0.0.0.0:8080 -t $HOME/ArrestDB
[Sun Jan 11 14:13:15 2026] PHP 8.2.29 Development Server (http://0.0.0.0:8080) started
```


## Test ArrestDB
```
$ curl "http://localhost:8080/customers/"
[
    {
        "id": "1",
        "name": "Luis",
        "gender": "1"
    },
    {
        "id": "2",
        "name": "Leonie",
        "gender": "1"
    },
    {
        "id": "3",
        "name": "Francois",
        "gender": "2"
    },
    {
        "id": "4",
        "name": "Bjorn",
        "gender": "2"
    }
]
```

---

# ESP32 Side


# Software requirements
ESP-IDF V5.0 or later.   
ESP-IDF V4.4 release branch reached EOL in July 2024.   

## Install
```
git clone https://github.com/nopnop2002/esp-idf-remote-sqlite3
cd esp-idf-remote-sqlite3/
idf.py menuconfig
idy.py flash monitor
```

You have to set this config value with menuconfig.   
- CONFIG_ESP_WIFI_SSID   
SSID of your wifi.
- CONFIG_ESP_WIFI_PASSWORD   
PASSWORD of your wifi.
- CONFIG_ESP_MAXIMUM_RETRY   
Maximum number of retries when connecting to wifi.
- CONFIG_ESP_WEB_SERVER_IP   
IP or mDNS host name of your WEB Server.
- CONFIG_ESP_WEB_SERVER_PORT   
Port number of your WEB Server.

![menuconfig-1](https://user-images.githubusercontent.com/6020549/97775496-79623d80-1ba4-11eb-99cc-1b309aa1689b.jpg)
![menuconfig-2](https://user-images.githubusercontent.com/6020549/160274237-2b87a981-f8e8-481b-9d01-77675ea58306.jpg)

## Read all data
```
I (3241) SQLITE: -----------------------------------------
I (3251) SQLITE: 1      Luis    1
I (3251) SQLITE: 2      Leonie  1
I (3251) SQLITE: 3      Francois        2
I (3261) SQLITE: 4      Bjorn   2
I (3261) SQLITE: -----------------------------------------
```

## Read by ID
```
I (4331) SQLITE: -----------------------------------------
I (4331) SQLITE: 3      Francois        2
I (4331) SQLITE: -----------------------------------------
```

## Read by gender
```
I (5141) SQLITE: -----------------------------------------
I (5141) SQLITE: 3      Francois        2
I (5141) SQLITE: 4      Bjorn   2
I (5151) SQLITE: -----------------------------------------
```

## Create new record
```
I (5891) SQLITE: -----------------------------------------
I (5901) SQLITE: 5      Tom     1
I (5901) SQLITE: -----------------------------------------
```

## Update record
```
I (18131) SQLITE: -----------------------------------------
I (18141) SQLITE: 5     Petty   2
I (18141) SQLITE: -----------------------------------------
```

## Delete record
```
I (31881) SQLITE: -----------------------------------------
I (31881) SQLITE: 1     Luis    1
I (31881) SQLITE: 2     Leonie  1
I (31891) SQLITE: 3     Francois        2
I (31891) SQLITE: 4     Bjorn   2
I (31891) SQLITE: -----------------------------------------
```


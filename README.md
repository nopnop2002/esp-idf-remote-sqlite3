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

For example, if the database is /home/nop/esp-idf-remote-sqlite3/sqlite/example.db, it will be as follows.

$dsn = 'sqlite:///home/nop/esp-idf-remote-sqlite3/sqlite/example.db';

```

## Install PHP
```
$ cd $HOME
$ sudo apt install php

$ php --version
PHP 7.2.24-0ubuntu0.18.04.1 (cli) (built: Oct 28 2019 12:07:07) ( NTS )
Copyright (c) 1997-2018 The PHP Group
Zend Engine v3.2.0, Copyright (c) 1998-2018 Zend Technologies
    with Zend OPcache v7.2.24-0ubuntu0.18.04.1, Copyright (c) 1999-2018, by Zend Technologies
```

## Install PDO driver for Sqlite3
```
$ sudo apt-get install php[Version]-sqlite3
```

When the PHP version is 7.2.x, execute the following.   
```
$ sudo apt-get install php7.2-sqlite3
```


## Start Built-in WEB Server
```
$ php -S 0.0.0.0:8080 -t $HOME/ArrestDB
PHP 7.2.24-0ubuntu0.18.04.7 Development Server started at Sat Oct 31 18:00:29 2020
Listening on http://0.0.0.0:8080
Document root is /home/nop/ArrestDB
Press Ctrl-C to quit.
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


## Software requirements
esp-idf ver4.1 or later.   

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
IP or DNS of your WEB Server.
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


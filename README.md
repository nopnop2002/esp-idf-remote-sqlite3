# esp-idf-remote-sqlite3
Remote-Sqlite access example for esp-idf.   
You can access Sqlite3 on the server over the network.   
Use [this](https://github.com/alixaxel/ArrestDB) as PHP script of WEB server.   

![remote-sqlite3](https://user-images.githubusercontent.com/6020549/97775795-c21af600-1ba6-11eb-9d02-04dcaca058c7.jpg)

# Server Side

## Create sqlite3 database   
Run the following script in any directory.   
```
#!/bin/bash
#set -x

if [ ! -x $(which sqlite3) ];then
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
$ git close https://github.com/alixaxel/ArrestDB
$ cd ArrestDB
$ vi index.php

Set the full path of the database in the following line.

$dsn = '';

For example, if the database is //home/nop/example.db, it will be as follows.

$dsn = 'sqlite:///home/nop/example.db';

```

## Install PHP
```

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
make menuconfig
make flash monitor
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
- CONFIG_JSON_PARSE   
Enable JSON parse.

![menuconfig-1](https://user-images.githubusercontent.com/6020549/97775496-79623d80-1ba4-11eb-99cc-1b309aa1689b.jpg)

![menuconfig-2](https://user-images.githubusercontent.com/6020549/97775498-7bc49780-1ba4-11eb-9d32-1984978f9b8b.jpg)

## Read all data
```
I (4557) SQLITE: [
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

## Read by ID
```
I (36197) SQLITE: {
    "id": "3",
    "name": "Francois",
    "gender": "2"
}
```

## Read by gender
```
I (65607) SQLITE: [
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

## Create new record
```
I (83577) SQLITE: {
    "id": "5",
    "name": "Tom",
    "gender": "1"
}
```

## Update record
```
I (100747) SQLITE: {
    "id": "5",
    "name": "Petty",
    "gender": "2"
}
```

## Delete record
```
I (140287) SQLITE: [
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


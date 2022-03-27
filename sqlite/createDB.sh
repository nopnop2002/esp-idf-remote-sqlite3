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

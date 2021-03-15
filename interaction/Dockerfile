FROM python:3.8-alpine

ENV TERM linux

run apk update && apk add --no-cache netcat-openbsd bash

COPY requirements.txt /
RUN pip install -r /requirements.txt

COPY exploit1.py exploit2.sh /
COPY check1.py check2.sh /


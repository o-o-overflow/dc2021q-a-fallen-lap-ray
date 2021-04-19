FROM --platform=linux/amd64 python:3.8-alpine

ENV TERM linux

run apk update && apk add --no-cache netcat-openbsd bash

COPY requirements.txt /
RUN pip install -r /requirements.txt

COPY exploit.py /
COPY check.py /


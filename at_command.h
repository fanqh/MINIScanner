/*
    Warning - this file was autogenerated by genparse
    DO NOT EDIT - any changes will be lost
*/

#ifndef AT_COMMAND_H
#define AT_COMMAND_H

#include <message_.h>

#ifdef __XAP__
#include <source.h>
#endif

const uint8 *parseData(const uint8 *s, const uint8 *e, Task task);
void handleUnrecognised(const uint8 *data, uint16 length, Task task);

#ifdef __XAP__
uint16 parseSource(Source rfcDataIncoming, Task task);
#endif

struct sequence
{
  const uint8 *data;
  uint16 length;
};

struct connect
{
  uint16 baudrate;
  uint16 parity;
  uint16 stop;
};
void connect(Task , const struct connect *);

#endif

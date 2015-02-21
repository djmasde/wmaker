#ifndef COMMANDS_H
#define COMMANDS_H

BOOL HelpCmd(proplist_t pl, int index);

BOOL ListCmd(proplist_t pl, int index);

BOOL AuthCmd(proplist_t pl, int index);

BOOL GetCmd(proplist_t pl, int index);

BOOL SetCmd(proplist_t pl, int index, BOOL skip);

BOOL RemoveCmd(proplist_t pl, int index, BOOL skip);

BOOL RegisterCmd(proplist_t pl, int index);

BOOL UnregisterCmd(proplist_t pl, int index);

BOOL UnknownCmd(proplist_t pl, int index);

#endif /* COMMANDS_H */

```
 .\build\tests\todolist\Debug\todolist_test.exe
Running 1 test case...

*** No errors detected
Detected memory leaks!
Dumping objects ->
{7973} normal block at 0x000001F1972FFD10, 16 bytes long.
 Data: < Oh<            > A0 4F 68 3C F7 7F 00 00 01 00 00 00 CD CD CD CD
Object dump complete.
```
The memory leak should be the msg disposer COM object.

```
Running 1 test case...

*** No errors detected
Detected memory leaks!
Dumping objects ->
{8040} normal block at 0x0000018ECEC22D70, 16 bytes long.
 Data: < o(             > A8 6F 28 B7 F7 7F 00 00 01 00 00 00 CD CD CD CD
{8038} normal block at 0x0000018ECEC22B90, 16 bytes long.
 Data: <Hp(             > 48 70 28 B7 F7 7F 00 00 01 00 00 00 CD CD CD CD
{8036} normal block at 0x0000018ECEC231D0, 16 bytes long.
 Data: < p(             > 08 70 28 B7 F7 7F 00 00 01 00 00 00 CD CD CD CD
Object dump complete.
```
When fabtric transport failed with CannotConnect, the eventhandler, notification handler, and msg disposer can leak
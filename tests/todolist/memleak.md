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
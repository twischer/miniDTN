INGA regression tests
===

Manual call:

    ./tools/profiling/test/test.py -n examples/inga_tests/node_config.yaml -t examples/inga_tests/sensor-tests/test_config.yaml

Each subfolder contains collection of tests for a specific domain.

coffee-tests
---
Tests for the COFFEE file system driver


fat-tests
---
Tests for the FAT file system driver


net-tests
---
Tests for network stacks like RIME, UDP, TCP


sensor-tests
---
Tests for the INGA sensor drivers


timer-tests
---
Tests for contiki timers (etimer, rtimer, ...)



Naming convention
---
Testfolders are named `CATEGORY-tests` where CATEGORY is one of
`timer`, `sensor`, `net`, `fat`, `coffee`, ...

Testfiles are named `NAME-test` where NAME is the name of the test.

Tests in testfolders are named `CATEGORY-NAME`.

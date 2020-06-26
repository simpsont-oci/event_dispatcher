[![Azure DevOps](https://dev.azure.com/simpsont0534/event_dispatcher/_apis/build/status/simpsont-oci.event_dispatcher)](https://dev.azure.com/simpsont0534/event_dispatcher/_build/latest?definitionId=1)

# Event Dispatcher (POC)

## Dependencies

- CMake
- Boost Development Libraries (findable by CMake)
- ACE/TAO (findable via OpenDDS by CMake)
- GoogleTest

## Getting Started

Even though this code does not directly depend on OpenDDS libraries, it makes use of the OpenDDS CMake module in order to build those files which depend on ACE/TAO. As a result, you'll want to run CMake with an additional prefix path, pointed to `${OPENDDS_ROOT}` as in the example below.

```
cmake -DCMAKE_PREFIX_PATH=<path_to_OpenDDS> .
make
./event_test
```



## Build

    git clone git@github.com:cpp-core/sort
    mkdir sort/build && cd sort/build
    CC=clang-15 CXX=clang++-15 cmake -DCMAKE_INSTALL_PREFIX=$HOME/opt ..
    make sort1

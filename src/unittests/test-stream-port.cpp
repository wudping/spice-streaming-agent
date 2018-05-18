/* The unit test for the low-level stream port IO.
 *
 * \copyright
 * Copyright 2018 Red Hat Inc. All rights reserved.
 */

#define CATCH_CONFIG_MAIN
#include <catch/catch.hpp>
#include <sys/socket.h>

#include "stream-port.hpp"
#include "error.hpp"


namespace ssa = spice::streaming_agent;

/*
 * Note the semantics of a socketpair may be different from the virtio port
 * that is actually used for the real interface.
 */
SCENARIO("test basic IO on the stream port", "[port][io]") {
    GIVEN("An open port (socketpair)") {
        int fd[2];
        const char *src_buf = "brekeke";
        const size_t src_size = strlen(src_buf);

        socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);

        WHEN("reading data in one go") {
            CHECK(write(fd[0], src_buf, src_size) == src_size);
            char buf[10];
            ssa::read_all(fd[1], buf, src_size);
            CHECK(std::string(buf, src_size) == src_buf);
        }

        WHEN("reading data in two steps") {
            CHECK(write(fd[0], src_buf, src_size) == src_size);
            char buf[10];
            ssa::read_all(fd[1], buf, 3);
            CHECK(std::string(buf, 3) == "bre");
            ssa::read_all(fd[1], buf, 4);
            CHECK(std::string(buf, 4) == "keke");
        }

        WHEN("writing data") {
            ssa::write_all(fd[1], src_buf, src_size);
            char buf[10];
            CHECK(read(fd[0], buf, src_size) == src_size);
            CHECK(std::string(buf, src_size) == src_buf);
        }

        WHEN("closing the remote end and trying to read") {
            CHECK(write(fd[0], src_buf, src_size) == src_size);
            char buf[10];
            ssa::read_all(fd[1], buf, 3);
            CHECK(std::string(buf, 3) == "bre");
            CHECK(close(fd[0]) == 0);
            ssa::read_all(fd[1], buf, 4);
            CHECK(std::string(buf, 4) == "keke");
            CHECK_THROWS_AS(ssa::read_all(fd[1], buf, 1), ssa::ReadError);
        }

        // clean up the descriptors in case they are still open
        close(fd[0]);
        close(fd[1]);
    }
}

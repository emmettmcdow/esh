FROM rockylinux:8 as base

RUN /usr/bin/dnf group install -y "Development Tools"
RUN /usr/bin/dnf install -y which xz

# Installing my favorite editor - for debugging
WORKDIR /opt
RUN /usr/bin/curl -L -O https://github.com/helix-editor/helix/releases/download/24.03/helix-24.03-x86_64-linux.tar.xz
RUN /usr/bin/tar xf helix-24.03-x86_64-linux.tar.xz
RUN /usr/bin/ln -s helix-24.03-x86_64-linux/hx /usr/bin/hx
RUN /usr/bin/rm -f helix-24.03-x86_64-linux.tar.xz

# Installing musl for a static binary
RUN /usr/bin/curl -O https://musl.libc.org/releases/musl-1.2.5.tar.gz
RUN /usr/bin/tar xzf musl-1.2.5.tar.gz
WORKDIR /opt/musl-1.2.5
RUN ./configure && make install
RUN /usr/bin/ln -s /usr/local/musl/bin/musl-gcc /usr/bin/musl-gcc
RUN /usr/bin/rm -f /opt/musl-1.2.5.tar.gz

# This has everything, including normal bash
FROM base as dev
WORKDIR /root/esh
ENTRYPOINT ["/bin/sh"]

# This target has no shell except for esh
FROM base as test
WORKDIR /root
RUN rm -f /usr/bin/sh /usr/bin/bash
ENTRYPOINT ["/bin/esh"]

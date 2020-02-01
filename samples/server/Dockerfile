#!docker

FROM ubuntu:latest

RUN apt-get update && \
    apt-get -y install \
        g++ git make autoconf \
        libexpat-dev libssl1.0-dev libsasl2-dev libldap-dev unixodbc-dev \
        liblua5.3-dev libv8-dev libncurses-dev libavformat-dev libswscale-dev \
        libgsm1-dev libspeex-dev libopus-dev libx264-dev libvpx-dev \
        libspandsp-dev capiutils dahdi

WORKDIR /usr/local/src
RUN git clone git://git.code.sf.net/p/opalvoip/ptlib && \
    git clone git://git.code.sf.net/p/opalvoip/opal
RUN make -C ptlib config && \
    make -C ptlib && \
    make -C ptlib install
RUN make -C opal config && \
    make -C opal && \
    make -C opal install
RUN make -C opal/samples/server && \
    make -C opal/samples/server install
RUN rm -rf ptlib opal

ARG opalsrv_gid=1917
ARG opalsrv_uid=1917
ARG username=opalsrv

RUN groupadd --gid ${opalsrv_gid} ${username} && \
    useradd  --gid ${opalsrv_gid} --uid ${opalsrv_uid} ${username}
USER ${username}

EXPOSE 1719/tcp            # Interface mini HTTP server
EXPOSE 1719/udp 1720/tcp   # H.323
EXPOSE 5060/tcp 5060/udp   # SIP
EXPOSE 5000-5999/udp       # RTP

WORKDIR /opt/opalsrv

ENTRYPOINT [ "/opt/opalsrv/bin/opalsrv", "--console", "--execute" ]

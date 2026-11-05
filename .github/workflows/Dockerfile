ARG BASE_IMAGE
FROM ${BASE_IMAGE}

ENV TZ America/Chicago
ENV DEBIAN_FRONTEND=noninteractive

ARG LLVM_VERSION
ARG JDK_VERSION

RUN     apt-get update                    \
    &&  apt-get upgrade -y                \
    &&  apt-get install -y                \
          git                             \
          cmake                           \
          clang-${LLVM_VERSION}           \
          clang-tidy-${LLVM_VERSION}      \
          debhelper                       \
          llvm-${LLVM_VERSION}-tools      \
          lld-${LLVM_VERSION}             \
          zlib1g-dev                      \
          flex                            \
          locales                         \
          libboost-dev                    \
          libboost-test-dev               \
          libfmt-dev                      \
          libgmp-dev                      \
          libmpfr-dev                     \
          libjemalloc-dev                 \
          libunwind-dev                   \
          libyaml-dev                     \
          openjdk-${JDK_VERSION}-jdk      \
          curl                            \
          maven                           \
          pkg-config                      \
          pipx                            \
          python3                         \
          python3-dev                     \
          python3-pybind11                \
          xxd

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

# The image is built specifically for an environment with this user/group
ARG USER=github-user
ARG GROUP=$USER
ARG USER_ID=1000
ARG GROUP_ID=$USER_ID
# Noble images ship an `ubuntu` user with uid 1000, which would clash.
RUN    (userdel --remove ubuntu 2>/dev/null || true) \
    && groupadd -g $GROUP_ID $GROUP && useradd -m -u $USER_ID -s /bin/sh -g $GROUP $USER

USER $USER:$GROUP
# The sources are copied in here, see .github/actions/with-docker.
RUN mkdir /home/$USER/workspace
WORKDIR /home/$USER/workspace

# lit 23 refuses the external shell that test/lit.cfg.py asks for (ShTest(True)),
# and PyPI has no release between 18.1.8 and 23.
RUN pipx install lit==18.1.8

ADD matching/pom.xml /home/user/.tmp-maven/
RUN    cd /home/user/.tmp-maven \
    && mvn -B dependency:go-offline

# Set $PATH to refer to LLVM's lit correctly
ENV PATH "$PATH:/home/$USER/.local/bin"

FROM amazonlinux:2023 AS build

RUN yum install -y clang make

WORKDIR /

COPY Makefile .
COPY src src

RUN mkdir target
RUN make target/hungry-search

FROM amazonlinux:2023 AS final

RUN yum install -y aws-cli && yum clean all

COPY --from=build target/hungry-search .

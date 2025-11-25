FROM ubuntu:latest
RUN apt-get update && apt-get install -y util-linux
WORKDIR /flock
COPY script.sh /script.sh
RUN chmod +x /script.sh
VOLUME /flock
CMD ["/script.sh"]

FROM debian
EXPOSE 8303/udp
ENV PATH="/usr/games/XXLDDRACE:$PATH"
RUN apt-get update && apt-get install -y \
   bam \
   build-essential \
   python
COPY . /usr/games/XXLDDRACE
WORKDIR /usr/games/XXLDDRACE
RUN bam server_release \
   && rm storage.cfg \
   && bash -c "echo add_path /usr/games/XXLDDRACE/config >> storage.cfg" \
   && bash -c "echo add_path /usr/games/XXLDDRACE/config/data >> storage.cfg"

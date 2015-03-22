FROM debian
MAINTAINER xxltomate <xxl-clan.com>
ENTRYPOINT ["XXLDDRace-Server_64"]
# forward this to the external port you wanna use 
EXPOSE 8303/udp
ENV PATH="/usr/games/XXLDDRACE:$PATH"
# install dependencies
RUN apt-get update && apt-get install -y \
   bam \
   build-essential \
   python
# get latest source
COPY . /usr/games/XXLDDRACE
WORKDIR /usr/games/XXLDDRACE
# compile source
RUN bam server_release \
# let server see that config files
   && rm storage.cfg \
   && bash -c "echo add_path /usr/games/XXLDDRACE/config >> storage.cfg" \
   && bash -c "echo add_path /usr/games/XXLDDRACE/config/data >> storage.cfg" \
# cleanup (uninstall tools which are only necessary to compile) 
   && apt-get purge -y \ 
      bam \
      build-essential \
      python \
   && apt-get autoremove -y

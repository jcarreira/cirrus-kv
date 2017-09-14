

To install docker:
(instructions from here https://docs.docker.com/engine/installation/linux/docker-ce/ubuntu/#install-using-the-repository)

~~~
sudo apt-get update
sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
sudo apt-get install docker-ce
sudo docker run hello-world
~~~

To run an empty docker container to verify RDMA function (make sure either eth2 or eth3, the RoCE interfaces, is up):

~~~
sudo docker run --net=host --ulimit memlock=-1 --device=/dev/infiniband/uverbs0 --device=/dev/infiniband/uverbs1 --device=/dev/infiniband/rdma_cm -t -i ubuntu /bin/bash
~~~

run runs the specified command (/bin/bash) inside of a container it creates
~~~
—net=host allows the container to use the host’s network stack
--ulimit memlock=-1 I’m not fully sure, but it allows ibv_reg_mr to succeed
—device adds an entry to /dev/infiniband/uverbs0 inside of the container
-t tty mode, allows you to disconnect from a running container with CTRL-p CTRL-q
-i specifies interactive mode, not sure if this is actually necessary
ubuntu the image to be run
/bin/bash the command being run
~~~

To run cirrus (tcpservermain specifically) inside of a container:
1. Create a directory called “docker” (name doesn’t really matter)
2. Place the attached ‘Dockerfile’ in the directory
3. git clone cirrus inside this directory (docker/cirrus)
4. switch to the docker branch (it comments out some parts of compilation. Later this could be changed to compile only the server or only the client/tests to allow for more lean containers)
5. run `sudo docker build -t cirrus_tcp_server`

at this point, the docker image is created, and the tcp server can be launched via (i saw an error about configs here but it still ran fine):

~~~
sudo docker run --net=host -t -i cirrus_tcp_server
~~~

the server can then be stopped via `sudo docker stop CONTAINER_NAME` (I’ve done this from another ssh session)

With this setup the container shares an ip with the host, so multiple servers on one host would need to be differentiated via port.


~~~
FROM ubuntu                                                                        
                                                                                   
WORKDIR /app                                                                       
ADD . /app                                                                         
                                                                                   
WORKDIR ./cirrus                                                                   
                                                                                   
RUN apt-get update && apt-get install -y build-essential git wget autoconf libtool g++-5 libboost-all-dev cmake gnulib python-pip && pip install cpplint 
RUN ./bootstrap.sh                                                                 
RUN make -j 10                                                                     
CMD ["./src/server/tcpservermain"]
~~~



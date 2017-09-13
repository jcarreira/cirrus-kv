CONTAINER_NAME=$1                                                                  
IP=$2                                                                              
                                                                                   
PACKAGES="build-essential,autoconf,libtool,g++-5,libboost-all-dev,cmake,python-pip,wget,git,gnulib,ca-certificates"
echo $CONTAINER_NAME                                                               
echo $PACKAGES                                                                     
lxc-create -t ubuntu -n $CONTAINER_NAME -- --packages $PACKAGES                    
                                                                                   
HOST_CONFIG_PATH="/var/lib/lxc/$CONTAINER_NAME/config"                             
echo $HOST_CONFIG_PATH                                                             
CONTAINER_CONFIG_PATH="/var/lib/lxc/$CONTAINER_NAME/rootfs/etc/network/interfaces"
                                                                                   
# Set up the configuration file on the host side                                   
# Remove the last line of the config as it will be replaced                        
sed -i '$ d' $HOST_CONFIG_PATH                                                     
                                                                                   
cat <<EOT >> $HOST_CONFIG_PATH                                                     
lxc.network.name = eth0                                                            
                                                                                   
lxc.network.type = veth                                                            
lxc.network.link = br0                                                             
lxc.network.flags = up                                                             
lxc.network.ipv4 = $IP                                                             
lxc.network.name = eth1                                                            
EOT                                                                                
                                                                                   
# Set up the configuration file on the container                                   
                                                                                   
cat <<EOT >> $CONTAINER_CONFIG_PATH                                                
                                                                                   
auto eth1                                                                          
iface eth1 inet static                                                             
address $IP                                                                    
broadcast 10.10.49.255                                                         
netmask 255.255.255.0                                                          
gateway 169.229.48.1                                                           
EOT                                                                                
                                                                                                   
# Launch the container                                                             
lxc-start -n $CONTAINER_NAME                                                       
                                                                                                   
# Setup that can only be done from within the container                            
# lxc-attach -n $CONTAINER_NAME -- apt-get install --reinstall ca-certificates  
lxc-attach -n $CONTAINER_NAME -- mkdir /usr/local/share/ca-certificates/cacert.org
lxc-attach -n $CONTAINER_NAME -- wget -P /usr/local/share/ca-certificates/cacert.org http://www.cacert.org/certs/root.crt http://www.cacert.org/certs/class3.crt
lxc-attach -n $CONTAINER_NAME -- update-ca-certificates                            
lxc-attach -n $CONTAINER_NAME -- pip install cpplint                               
                                                                

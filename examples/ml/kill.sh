PS_IP="172.31.6.154"
KEY="~/.ssh/id_rsa2"

ssh -i $KEY $PS_IP "killall parameter_server"

name=ps_output-`date +"%F_%T"`
#if [[ -e $name ]] ; then
#  i = 0
#  while [[ -e $name-$i ]] ; do
#    let i++
#  done
#  name=$name-$i
#fi

cmd="cp ps_output $name"
echo $cmd
ssh -i $KEY $PS_IP $cmd



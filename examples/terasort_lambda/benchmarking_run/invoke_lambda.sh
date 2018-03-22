NUM_WORKERS=10
COUNTER=0
while [ $COUNTER -lt $NUM_WORKERS ]; do
  echo Launching worker with id: $COUNTER
  cmd="aws lambda invoke --invocation-type RequestResponse --function-name cirrus_terasort --region us-west-2 --log-type Tail --payload '{\"proc_num\":\""$COUNTER\""}' outputfile$COUNTER.txt&"
  echo "cmd: $cmd"
  eval $cmd
  let COUNTER=COUNTER+1 
done

wait;

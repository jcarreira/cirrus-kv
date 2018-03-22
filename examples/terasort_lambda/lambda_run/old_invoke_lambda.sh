NUM_WORKERS=30
HASH_WORKERS=20
COUNTER=0
while [ $COUNTER -lt $HASH_WORKERS ]; do
  echo Launching worker with id: $COUNTER
  cmd="aws lambda invoke --invocation-type RequestResponse --function-name cirrus_terasort_hasher --region us-west-2 --log-type Tail --payload '{\"proc_num\":\""$COUNTER\""}' outputfile$COUNTER.txt&"
  echo "cmd: $cmd"
  eval $cmd
  let COUNTER=COUNTER+1 
done

while [ $COUNTER -lt $NUM_WORKERS ]; do
  echo Launching worker with id: $COUNTER
  cmd="aws lambda invoke --invocation-type RequestResponse --function-name cirrus_terasort_sorter --region us-west-2 --log-type Tail --payload '{\"proc_num\":\""$COUNTER\""}' outputfile$COUNTER.txt&"
  echo "cmd: $cmd"
  eval $cmd
  let COUNTER=COUNTER+1 
done

wait

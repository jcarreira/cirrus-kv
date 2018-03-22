NUM_WORKERS=30
HASH_WORKERS=20
COUNTER=0

function hash_lambda() {
  ID=$1;
  DONE="HASH_DONE_FLAG_$ID"
  while true; do
    if aws s3 ls s3://cirrus-terasort-bucket | grep $DONE 2> /dev/null > /dev/null; then
      break
    fi
    echo "Starting hash process $ID"
    cmd="aws lambda invoke --invocation-type RequestResponse --function-name cirrus_terasort_hasher --region us-west-2 --log-type Tail --payload '{\"proc_num\":\""$ID\""}' outputfile$ID.txt"
    echo "cmd: $cmd"
    eval $cmd
  done
}

function sort_lambda() {
  ID=$1;
  ((DONE_ID = ID - HASH_WORKERS))
  DONE="SORT_DONE_FLAG_$DONE_ID"
  while true; do
    if aws s3 ls s3://cirrus-terasort-bucket | grep $DONE 2> /dev/null > /dev/null; then
      break
    fi
    echo "Starting sort process $ID"
    cmd="aws lambda invoke --invocation-type RequestResponse --function-name cirrus_terasort_sorter --region us-west-2 --log-type Tail --payload '{\"proc_num\":\""$ID\""}' outputfile$ID.txt"
    echo "cmd: $cmd"
    eval $cmd
  done
}

while [ $COUNTER -lt $HASH_WORKERS ]; do
  hash_lambda $COUNTER &
  ((COUNTER = COUNTER + 1))
done

while [ $COUNTER -lt $NUM_WORKERS ]; do
  sort_lambda $COUNTER &
  ((COUNTER = COUNTER + 1))
done

wait

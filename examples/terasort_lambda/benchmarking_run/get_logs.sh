rm -f log

aws logs filter-log-events \
      --log-group-name /aws/lambda/cirrus_terasort \
      --output text > log || rm -f log

aws logs delete-log-group --log-group-name /aws/lambda/cirrus_terasort

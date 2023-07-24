#! /bin/bash

d=$(dirname $0)
c=$d/credentials
p=$$

echo - Configure AWS API credentials
aws configure

f_cred=$d/mqtt_pub_sub/config_files/aws_clientcredential_keys.h
f_config=$d/mqtt_pub_sub/config_files/demo_config.h


echo - Create thing credentials
key_prv=$c/thing.prv.$p.key
key_pub=$c/thing.pub.$p.key
crt_509=$c/thing.crt.$p.crt

mkdir -p $c

aws iot create-keys-and-certificate \
	--set-as-active \
	--certificate-pem-outfile $crt_509 \
	--public-key-outfile      $key_pub \
	--private-key-outfile     $key_prv > $c/certificate.$p.config

sed -e 's/^/"/g' -e 's/$/\\n"\\/g' -e '$ s/.$//' $crt_509 > $crt_509.block
sed -e 's/^/"/g' -e 's/$/\\n"\\/g' -e '$ s/.$//' $key_prv > $key_prv.block

sed -e 's/keyCLIENT_CERTIFICATE_PEM ""/keyCLIENT_CERTIFICATE_PEM/' \
    -e "/keyCLIENT_CERTIFICATE_PEM/r $crt_509.block" $f_cred | \
    sed -e ':a;N;$!ba;s/keyCLIENT_CERTIFICATE_PEM\n/keyCLIENT_CERTIFICATE_PEM /g' > \
      /tmp/k$p && mv -f /tmp/k$p $f_cred

sed -e 's/keyCLIENT_PRIVATE_KEY_PEM ""/keyCLIENT_PRIVATE_KEY_PEM/' \
    -e "/keyCLIENT_PRIVATE_KEY_PEM/r $key_prv.block" $f_cred | \
    sed -e ':a;N;$!ba;s/keyCLIENT_PRIVATE_KEY_PEM\n/keyCLIENT_PRIVATE_KEY_PEM /g' > \
      /tmp/k$p && mv -f /tmp/k$p $f_cred

clientid=$(hostname | sed -e 's/-//g')-$p
endpoint=$(aws iot describe-endpoint --endpoint-type iot:Data-ATS --output text)
certificate_arn=$(jq -r .certificateArn $d/credentials/certificate.$p.config)
certificate_id=$(jq -r .certificateId $d/credentials/certificate.$p.config)

aws iot create-thing --thing-name $clientid > $c/thing.$p.config
aws iot attach-thing-principal \
  --thing-name $clientid \
  --principal $certificate_arn
aws iot attach-policy \
  --policy-name dev_policy \
  --target $certificate_arn

sed -i -e "s/^#define democonfigCLIENT_IDENTIFIER.*/#define democonfigCLIENT_IDENTIFIER \"$clientid\"/" $f_config
sed -i -e "s/^\s#define democonfigMQTT_BROKER_ENDPOINT.*/#define democonfigMQTT_BROKER_ENDPOINT \"$endpoint\"/" $f_config



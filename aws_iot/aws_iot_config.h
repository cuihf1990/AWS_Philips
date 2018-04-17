/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file aws_iot_config.h
 * @brief AWS IoT specific configuration file
 */

#ifndef AWS_IOT_CONFIG_H_
#define AWS_IOT__CONFIG_H_

// Get from console
// =================================================
#define AWS_IOT_MQTT_HOST              "a1lqshc4oegz64.iot.us-west-2.amazonaws.com" ///< Customer specific MQTT HOST. The same will be used for Thing Shadow
#define AWS_IOT_MQTT_PORT              8883 ///< default port for MQTT/S
#define AWS_IOT_MQTT_CLIENT_ID         "MICO" ///< MQTT client ID should be unique for every device
#define AWS_IOT_MY_THING_NAME          "Mylight" ///< Thing Name of the Shadow this device is associated with

#define AWS_IOT_CERTIFICATE_FILENAME   "\
-----BEGIN CERTIFICATE-----\n\
MIIDWTCCAkGgAwIBAgIUVtsDRUEJ8Hb8OqE35C4GKr9MQCQwDQYJKoZIhvcNAQEL\n\
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n\
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTE3MDMyMjAzNDg0\n\
M1oXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n\
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKpKGTwnVu+40zto4nHN\n\
XlH8yWJswrv3YS4FeYYBKTleHzOx7exjdXMd/L019cmUExPinKKc9wlPIjg5YlYT\n\
0qTgNIqoN0KSwr2lNxfrl7BhlGwNjv1n78JhNgq3JSHXXwbjx4TpSvDVi1Xk2gOQ\n\
8WQAJyf7PwcArHtVWJf3S/nbxsdywT9GxK7otF6i59fWe2gvCrMUgJ0sN48sm9d3\n\
kvQba+P5urWGOZoIi7VkGucQxogvIp7pu8tWtKwfF3qfLERGhhTMHEfnU6NlJaVn\n\
I5gRUQzo2ZD25ly2nsSyaYVaPjzCykDAk/VR4HlmXhl9LlP5nJLZeFR7wz369qAY\n\
ml8CAwEAAaNgMF4wHwYDVR0jBBgwFoAUnx5icteUC/r3yxIPHFJE9+4SH5wwHQYD\n\
VR0OBBYEFJQkP39VqmzrH4aKLQex8ERFzvTQMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n\
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAgqp2O3YhDcROr5pAto/rpAhtP\n\
SQIsst8W2SMaeimt9w9j8VEhUvWVDaVEjYCDdqYM6eZqzPXSXSkOsX0hT6Sj+VKt\n\
Sqn7POVZYaeJOopSZFid5NQPUcZnDW8HSNUo58Ow3n3WLgNrucNwrCVMf8hsUHNc\n\
5XIEZ+7X04JsXQkQCqHIP5jJOOsi6TFsB03eiyYeFGAYG/UX9TL73+3QGt9ABUn/\n\
PhLtql6vN87LOApQQ8P4SudFXUQ5h1pkYK4dLhhrP1cdIT7F/w8RgEILgMsb2/nI\n\
Nmgha2YDDMMJrZivg982f6gmv4hI/k/FDv+f294H+JOk3coIF2b5CDSWoqJr\n\
-----END CERTIFICATE-----\n\
"
#define AWS_IOT_ROOT_CA_FILENAME "\
-----BEGIN CERTIFICATE-----\n\
MIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB\n\
yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\n\
ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp\n\
U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW\n\
ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0\n\
aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL\n\
MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW\n\
ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln\n\
biwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp\n\
U2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y\n\
aXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1\n\
nmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex\n\
t0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz\n\
SdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG\n\
BO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+\n\
rCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/\n\
NIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E\n\
BAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH\n\
BgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy\n\
aXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv\n\
MzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE\n\
p6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y\n\
5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK\n\
WE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ\n\
4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N\n\
hnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq\n\
-----END CERTIFICATE-----\n\
"

#define AWS_IOT_PRIVATE_KEY_FILENAME  "\
-----BEGIN RSA PRIVATE KEY-----\n\
MIIEowIBAAKCAQEAqkoZPCdW77jTO2jicc1eUfzJYmzCu/dhLgV5hgEpOV4fM7Ht\n\
7GN1cx38vTX1yZQTE+Kcopz3CU8iODliVhPSpOA0iqg3QpLCvaU3F+uXsGGUbA2O\n\
/WfvwmE2CrclIddfBuPHhOlK8NWLVeTaA5DxZAAnJ/s/BwCse1VYl/dL+dvGx3LB\n\
P0bErui0XqLn19Z7aC8KsxSAnSw3jyyb13eS9Btr4/m6tYY5mgiLtWQa5xDGiC8i\n\
num7y1a0rB8Xep8sREaGFMwcR+dTo2UlpWcjmBFRDOjZkPbmXLaexLJphVo+PMLK\n\
QMCT9VHgeWZeGX0uU/mcktl4VHvDPfr2oBiaXwIDAQABAoIBAEVnCb3gcqgk9cIi\n\
zxd+kdBsbE828G7XNb4h8RNSadC9sY3KGKPdLUMLl7Qtx8yuEtBp7VjBDFW48MNl\n\
b9SRI6qazg8s28jAM6pDKZ8QH7R9MJaROBPDRo48PGBFoFaYhlwyfWCIDEj3X2BU\n\
cx489oTBIzRCS7+44JMrh334BfkAlBDwhABt9yIGkrMWpSuOfzzx4EwK878XYpp/\n\
miPHbMlkvHIzaTHeLlgx39mHPTi41w4yWa7xk79NpsfXeYvSqB81ovxNZaBdD1mW\n\
RlciEU0so6ABW7MnMrQmAFX5o9c84jHRUiuhkBgtspGPdnBRkEHlMfLGTsM/rTee\n\
65tQ3GECgYEA7IZT4s815rQqXrAfqQA3PHfKAg+tXhA1WV1JHXE7S8dx7zYFqZwG\n\
bcskp2wVkf/wmgxbpSDqZJkZVjJhkE0hTL3dU060DuJ0EINmttSHxXmNvtY5whK8\n\
qTNksllD1h1be/remWHMwq2yN6YGgRDBfM+qgMyqIZQ11qje4a5FQHcCgYEAuE+b\n\
TdaK+hTCKVjuFxWKZtWzcQ1CrYY+j/fJMB6k25w1zfcy7WAzLRsLwzTmky1+csQU\n\
SDWkYLb4ebM+lB+XovjQHTC1GnjED8jbGvoet28jgiTY9Zj/biTin+Dv7dE5pqXE\n\
BpNGvpyezxMcT/j+2wXH2sXbSbOKcsVp8/25l1kCgYACzAfb68VgaAsEOaL2Nalx\n\
jp0V7yeGsDxhRSgjL/6ag85GHOZuPgkZbUiOrkmHA3bN641942jCLclN6qSatz7I\n\
kIo4fPrGHklvFI63ZMYCQNC7S/8820nd2ly7ezDBLHGzgqD1QWHRf4pW/CChkBgp\n\
qK8EfOcaJI2Kb07LbmslOwKBgGYqeoQjMNZ/O0GAMjpJjnaCbv1zxEo75+IWEEfE\n\
NM4nQQvywyAh+zw9ib+jS0y6IyWq2zNLyNpzkjijy0SAXqXQFkyX+0u5NbUqOYoy\n\
q2QzDxPNKRa6wJxlhdnp7hV9rN9bc9XRPZ3bY4yVo1QRSfROuAHlOzEXfN8x3xGI\n\
y5/BAoGBANZvlTd2uXuGWiJpIZXbN933Tal/XmvPdW0AFUdnyA8RJX7W8MrDbZSJ\n\
hQCUXvv/MNP8eIB1aSPROmbT/adK+3mEqMsBPgkqTn2TpL8O4/JEJYWPoWPOug1D\n\
PBRD0yXZ0eyVC9eELlLNBtPDiBIDLxr2ENjwd1Kad43lDC4yuWKe\n\
-----END RSA PRIVATE KEY-----\n\
"
// =================================================


// Thing Shadow specific configs
#define SHADOW_MAX_SIZE_OF_RX_BUFFER MQTT_RX_BUF_LEN+1 ///< Maximum size of the SHADOW buffer to store the received Shadow message
#define MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES 80  ///< Maximum size of the Unique Client Id. For More info on the Client Id refer \ref response "Acknowledgments"
#define MAX_SIZE_CLIENT_ID_WITH_SEQUENCE MAX_SIZE_OF_UNIQUE_CLIENT_ID_BYTES + 10 ///< This is size of the extra sequence number that will be appended to the Unique client Id
#define MAX_SIZE_CLIENT_TOKEN_CLIENT_SEQUENCE MAX_SIZE_CLIENT_ID_WITH_SEQUENCE + 20 ///< This is size of the the total clientToken key and value pair in the JSON
#define MAX_ACKS_TO_COMEIN_AT_ANY_GIVEN_TIME 10 ///< At Any given time we will wait for this many responses. This will correlate to the rate at which the shadow actions are requested
#define MAX_THINGNAME_HANDLED_AT_ANY_GIVEN_TIME 10 ///< We could perform shadow action on any thing Name and this is maximum Thing Names we can act on at any given time
#define MAX_JSON_TOKEN_EXPECTED 120 ///< These are the max tokens that is expected to be in the Shadow JSON document. Include the metadata that gets published
#define MAX_SHADOW_TOPIC_LENGTH_WITHOUT_THINGNAME 60 ///< All shadow actions have to be published or subscribed to a topic which is of the format $aws/things/{thingName}/shadow/update/accepted. This refers to the size of the topic without the Thing Name
#define MAX_SIZE_OF_THING_NAME 20 ///< The Thing Name should not be bigger than this value. Modify this if the Thing Name needs to be bigger
#define MAX_SHADOW_TOPIC_LENGTH_BYTES MAX_SHADOW_TOPIC_LENGTH_WITHOUT_THINGNAME + MAX_SIZE_OF_THING_NAME ///< This size includes the length of topic with Thing Name

#endif

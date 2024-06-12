import json
import signal
import argparse
import paho.mqtt.client as mqtt
from decouple import config

def on_connect(client, userdata, connect_flags, reason_code, properties):
    if reason_code.is_failure:
        print(f"Failed to connect, reason code {reason_code}")
    else:
        print("Connected to MQTT Broker")
        print("Subscribing ...")

        topics = [
            f"v3/{userdata.application_id}@{userdata.tenant_id}/devices/{userdata.device_id}/join",
            f"v3/{userdata.application_id}@{userdata.tenant_id}/devices/{userdata.device_id}/up",
            f"v3/{userdata.application_id}@{userdata.tenant_id}/devices/{userdata.device_id}/down/queued",
            f"v3/{userdata.application_id}@{userdata.tenant_id}/devices/{userdata.device_id}/down/sent",
            f"v3/{userdata.application_id}@{userdata.tenant_id}/devices/{userdata.device_id}/down/ack",
            f"v3/{userdata.application_id}@{userdata.tenant_id}/devices/{userdata.device_id}/down/nack",
            f"v3/{userdata.application_id}@{userdata.tenant_id}/devices/{userdata.device_id}/down/failed",
            f"v3/{userdata.application_id}@{userdata.tenant_id}/devices/{userdata.device_id}/service/data",
            f"v3/{userdata.application_id}@{userdata.tenant_id}/devices/{userdata.device_id}/location/solved"
        ]

        for topic in topics:
            client.subscribe(topic, userdata.qos)

def on_message(client, userdata, message):
    print("Message received ...")
    payload = message.payload.decode()

    try:
        data = json.loads(payload)
        if 'uplink_message' in data and 'decoded_payload' in data['uplink_message']:
            decoded_payload = data['uplink_message']['decoded_payload']
            print(f"@{message.topic}: {decoded_payload}")
            print()

    except json.JSONDecodeError:
        print(f"Failed to decode JSON from topic {message.topic}")


def on_unsubscribe(client, userdata, mid, reason_code_list, properties):
    # The reason_code_list is empty in MQTT v3
    if len(reason_code_list) == 0:
        print("Unsubscribe succeeded")
    else:
        print(f"Broker replied with failure: {reason_code_list[0]}")

    client.disconnect()


def parse_arguments():
    parser = argparse.ArgumentParser(
        prog="TTSSubscriber",
        description="Subscribe to The Things Stack (TTS) MQTT endpoint and print decoded payloads. "
                    "TTS_HOST, TTS_PORT, TTS_USERNAME, TTS_PASSWORD, TTS_APPLICATION_ID, TTS_TENANT_ID, "
                    "and TTS_DEVICE_ID must be provided in an '.env' file",
        epilog="Adapted from 'https://github.com/eclipse/paho.mqtt.python'"
    )
    parser.add_argument(
        "-q", "--qos",
        type=int,
        choices=[0, 1, 2],
        default=0,
        help="Tweak reliability/latency, default is 0"
    )
    parser.add_argument(
        "--persistent",
        action="store_true",
        help="Retain session data (subscriptions, outstanding messages) when this client disconnects"
    )

    args = parser.parse_args()

    args.host = config("TTS_HOST")
    args.port = config("TTS_PORT", cast=int)
    args.username = config("TTS_USERNAME")
    args.passwd = config("TTS_PASSWORD")
    args.application_id = config("TTS_APPLICATION_ID")
    args.tenant_id = config("TTS_TENANT_ID")
    args.device_id = config("TTS_DEVICE_ID")

    return args


def main():
    args = parse_arguments()

    # create the client instance and register the callbacks
    mqttc = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2, clean_session=not args.persistent)
    mqttc.user_data_set(args)
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message
    mqttc.on_unsubscribe = on_unsubscribe

    # connect to the MQTT broker
    mqttc.username_pw_set(args.username, args.passwd)
    mqttc.connect(host=args.host, port=args.port)
    mqttc.loop_start()

    # stop the mqttc loop upon receiving SIGINT
    def handler(sig, frame):
        mqttc.disconnect()
        mqttc.loop_stop()
        print("\nExiting ...")
        exit(0)

    signal.signal(signal.SIGINT, handler)

    while True:
        pass


if __name__ == "__main__":
    main()

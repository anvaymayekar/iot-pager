import paho.mqtt.client as mqtt
from datetime import datetime
from types import SimpleNamespace
import ssl
from time import time, sleep
import threading

"""
Program for Python acting as an MQTT publisher
author: @anvaymayekar
"""


# function to log the data with discrete hues
def echo(text: str, level: str = "") -> None:
    level_colors = {
        "BOOT": "magenta",
        "READY": "cyan",
        "SYNC": "blue",
        "TXLOG": "green",
        "CNFRM": "blue",
        "LOST": "yellow",
        "ERROR": "red",
        "RETRY": "yellow",
        "CLOSE": "cyan",
        "ABORT": "red",
        "DEBUG": "magenta",
    }

    ansi_colors = {
        "black": "\033[30m",
        "red": "\033[31m",
        "green": "\033[32m",
        "yellow": "\033[33m",
        "blue": "\033[34m",
        "magenta": "\033[35m",
        "cyan": "\033[36m",
        "white": "\033[37m",
    }

    level = level.upper().strip()

    if not level:
        print(f"\033[37m{text}\033[0m")  # plain white, no tag
    else:
        tag = f"[{level}]"
        color = ansi_colors.get(level_colors.get(level, "white"), "")
        print(f"{color}{tag} {text}\033[0m")


# MQTT Broker Credentials
credentials = SimpleNamespace(
    broker="your-broker-url.hivemq.cloud",
    port=8883,
    username="your-username",
    password="your-password",
    topic="telemetrix/stream",
    topic_status="status/esp32",
)

# Credentials for testing only — use secure methods (e.g., environmental variables) in production.


class Publisher:

    def __init__(self, credentials: SimpleNamespace) -> None:
        # configuration
        self.__broker = credentials.broker
        self.__port = credentials.port
        self.__username = credentials.username
        self.__password = credentials.password
        self.__topic = credentials.topic
        self.__status_topic = credentials.topic_status

        # internal states
        self.__lock = threading.Lock()
        self.__grace_period = 10
        self.__heartbeat_timeout = 20  # seconds
        self.__last_heartbeat = 0
        self.__running = False

    # MQTT client setup
    def __setup_client(self) -> None:
        self.__client = mqtt.Client()
        self.__client.username_pw_set(self.__username, self.__password)
        self.__client.tls_set(
            cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS_CLIENT
        )

        self.__client.on_connect = self._on_connect
        self.__client.on_disconnect = self._on_disconnect
        self.__client.on_message = self._on_message
        self.__client.on_publish = self._on_publish

    def __get_timestamp(self) -> str:
        timestamp: str = datetime.now().strftime("%d %b %y %H:%M:%S ")
        return timestamp

    def start(self) -> None:
        self.__setup_client()
        try:
            self.__client.connect(self.__broker, self.__port)
            self.__client.loop_start()
            echo("MQTT Messenger initialized.\n", "BOOT")
            echo(f"Connecting to broker at {self.__broker}", level="sync")
            sleep(2)
        except Exception as e:
            echo(f"Connection failed: {e}\n", "ERROR")

    def stop(self) -> None:
        self.__running = False
        try:
            self.__client.loop_stop()
            self.__client.disconnect()
            echo("MQTT client disconnected successfully.", level="CLOSE")
        except Exception as e:
            echo(f"Error during disconnect: {e}", level="ERROR")

    def __send(self, message) -> None:
        timestamp = self.__get_timestamp()
        payload: str = f"{timestamp}  {message[:147]}"
        result = self.__client.publish(self.__topic, payload)
        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            echo(f"[{timestamp}] Failed to publish message.", "ERROR")
        else:
            echo(f"[{timestamp}] Message published sucessfully!", "TXLOG")
            if time() - self.__last_heartbeat > self.__heartbeat_timeout:
                echo(
                    "ESP32 appears to be offline — message will be retained.\n",
                    "LOST",
                )
        sleep(0.5)

    def _monitor_heartbeat(self) -> None:
        offline_logged = False
        startup_time = time()
        while True:
            with self.__lock:
                elapsed = (
                    time() - self.__last_heartbeat
                    if self.__last_heartbeat
                    else float("inf")
                )

            # Grace period of 10 sec from startup
            if time() - startup_time < self.__grace_period:
                sleep(1)
                continue

            if elapsed > self.__heartbeat_timeout:
                if not offline_logged:
                    echo("ESP32 heartbeat lost — device might be offline.", "LOST")
                    offline_logged = True
            else:
                offline_logged = False  # reset if heartbeat returns

            sleep(5)

    def run(self) -> None:
        """Start interactive loop to send messages."""
        self.__running = True
        echo(
            "ESP32 interface ready — enter message or type 'exit' to terminate.\n",
            "READY",
        )
        sleep(1)
        try:
            while self.__running:
                message: str = input(">> ").strip()
                if message.lower() == "exit":
                    break
                if message:
                    self.__send(message)
        except KeyboardInterrupt:
            echo("\nInterrupt received. Cleaning up...")
        finally:
            self.stop()

    # MQTT callbacks

    def _on_connect(self, _client, _userdata, _flags, rc) -> None:
        if rc == 0:
            echo("Connected to MQTT broker.", "SYNC")
            self.__client.subscribe(self.__status_topic)
            self.__last_heartbeat = time()
            threading.Thread(target=self._monitor_heartbeat, daemon=True).start()
            sleep(1)

        else:
            echo(f"MQTT connection failed with code {rc}\n", "ERROR")

    def _on_disconnect(self, *_) -> None:
        echo("Session terminated by user.", level="ABORT")
        sleep(1)
        try:
            self.__client.reconnect()
        except:
            echo("Reconnection attempt failed.\n", "RETRY")

    def _on_message(self, client, userdata, msg) -> None:
        if msg.topic == self.__status_topic:
            with self.__lock:
                self.__last_heartbeat = time()

    def _on_publish(self, *_) -> None:
        echo("MQTT message acknowledged by broker.\n", "CNFRM")


# Entry Point
if __name__ == "__main__":
    pager = Publisher(credentials)
    pager.start()
    pager.run()

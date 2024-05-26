import json
import sqlite3
import telebot
from telebot import types
import paho.mqtt.client as mqtt

# Инициализация Telegram бота
bot = telebot.TeleBot('key')

# Настройки MQTT
MQTT_SERVER = "public.mqtthq.com"
MQTT_PORT = 1883

# Инициализация базы данных
conn = sqlite3.connect('user_data.db', check_same_thread=False)

# Создание таблицы пользователей, если она не существует
def create_db():
    with conn:
        cursor = conn.cursor()
        cursor.execute('''
        CREATE TABLE IF NOT EXISTS users (
            chat_id INTEGER PRIMARY KEY,
            topics TEXT,
            notifications TEXT
        )
        ''')

create_db()

# Загрузка данных пользователя из базы данных
def load_user_data(chat_id):
    with conn:
        cursor = conn.cursor()
        cursor.execute('SELECT topics, notifications FROM users WHERE chat_id = ?', (chat_id,))
        result = cursor.fetchone()
        if result:
            topics = json.loads(result[0])
            notifications = json.loads(result[1])
        else:
            topics = []
            notifications = {}
            cursor.execute('INSERT INTO users (chat_id, topics, notifications) VALUES (?, ?, ?)',
                           (chat_id, json.dumps(topics), json.dumps(notifications)))
            conn.commit()
    return topics, notifications

# Сохранение данных пользователя в базу данных
def save_user_data(chat_id, topics, notifications):
    with conn:
        cursor = conn.cursor()
        cursor.execute('UPDATE users SET topics = ?, notifications = ? WHERE chat_id = ?',
                       (json.dumps(topics), json.dumps(notifications), chat_id))
        conn.commit()

# Обратный вызов при получении сообщения от MQTT брокера
def on_message(client, userdata, message):
    topic = message.topic
    payload = message.payload.decode()
    print(f"Сообщение получено: Топик: {topic}, Сообщение: {payload}")
    with conn:
        cursor = conn.cursor()
        cursor.execute('SELECT chat_id, topics, notifications FROM users')
        for row in cursor.fetchall():
            chat_id = row[0]
            topics = json.loads(row[1])
            notifications = json.loads(row[2])
            if topic in topics:
                if notifications.get(topic, False):
                    bot.send_message(chat_id, f"Сообщение MQTT:\nТопик: {topic}\nСообщение: {payload}")

# Настройка MQTT клиента и подключение к брокеру
mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_SERVER, MQTT_PORT, 60)
mqtt_client.loop_start()

# Обработчики Telegram бота
@bot.message_handler(commands=['start'])
def send_welcome(message):
    chat_id = message.chat.id
    topics, notifications = load_user_data(chat_id)
    markup = types.ReplyKeyboardMarkup(resize_keyboard=True)
    btn_topics = types.KeyboardButton("/topics")
    btn_help = types.KeyboardButton("/help")
    markup.add(btn_topics, btn_help)
    bot.send_message(chat_id, "Добро пожаловать! Используйте /topics для управления вашими топиками. Используйте /help для просмотра всех команд.", reply_markup=markup)

@bot.message_handler(commands=['topics'])
def manage_topics(message):
    markup = types.ReplyKeyboardMarkup(resize_keyboard=True)
    btn_add = types.KeyboardButton("Добавить топик")
    btn_delete = types.KeyboardButton("Удалить топик")
    btn_toggle_notifications = types.KeyboardButton("Переключить уведомления")
    btn_list = types.KeyboardButton("Список топиков")
    markup.add(btn_add, btn_delete, btn_toggle_notifications, btn_list)
    bot.send_message(message.chat.id, "Управляйте вашими топиками:", reply_markup=markup)

@bot.message_handler(func=lambda message: message.text in ["Добавить топик", "Удалить топик", "Переключить уведомления", "Список топиков"])
def handle_topic_actions(message):
    if message.text == "Добавить топик":
        msg = bot.send_message(message.chat.id, "Отправьте мне топик, который хотите добавить:")
        bot.register_next_step_handler(msg, add_topic)
    elif message.text == "Удалить топик":
        msg = bot.send_message(message.chat.id, "Отправьте мне топик, который хотите удалить:")
        bot.register_next_step_handler(msg, delete_topic)
    elif message.text == "Переключить уведомления":
        msg = bot.send_message(message.chat.id, "Отправьте мне топик, для которого хотите переключить уведомления:")
        bot.register_next_step_handler(msg, toggle_notifications)
    elif message.text == "Список топиков":
        list_user_topics(message)

def add_topic(message):
    topic = message.text
    chat_id = message.chat.id
    topics, notifications = load_user_data(chat_id)
    if topic not in topics:
        topics.append(topic)
        notifications[topic] = True
        mqtt_client.subscribe(topic)
        save_user_data(chat_id, topics, notifications)
        bot.send_message(chat_id, f"Топик '{topic}' добавлен и подписан.")
    else:
        bot.send_message(chat_id, "Этот топик уже есть в вашем списке.")

def delete_topic(message):
    topic = message.text
    chat_id = message.chat.id
    topics, notifications = load_user_data(chat_id)
    if topic in topics:
        topics.remove(topic)
        if topic in notifications:
            del notifications[topic]
        mqtt_client.unsubscribe(topic)
        save_user_data(chat_id, topics, notifications)
        bot.send_message(chat_id, f"Топик '{topic}' удален и отписан.")
    else:
        bot.send_message(chat_id, "Этот топик не в вашем списке.")

def toggle_notifications(message):
    topic = message.text
    chat_id = message.chat.id
    topics, notifications = load_user_data(chat_id)
    if topic in topics:
        notifications[topic] = not notifications.get(topic, True)
        state = "включены" if notifications[topic] else "выключены"
        save_user_data(chat_id, topics, notifications)
        bot.send_message(chat_id, f"Уведомления для топика '{topic}' теперь {state}.")
    else:
        bot.send_message(chat_id, "Этот топик не в вашем списке.")

def list_user_topics(message):
    chat_id = message.chat.id
    topics, notifications = load_user_data(chat_id)
    if topics:
        topics_list = '\n'.join(topics)
        bot.send_message(chat_id, f"Ваши топики:\n{topics_list}")
    else:
        bot.send_message(chat_id, "У вас нет топиков.")

@bot.message_handler(commands=['help'])
def send_help(message):
    help_text = (
        "Доступные команды:\n"
        "/start - Инициализация бота и начало управления топиками.\n"
        "/topics - Управление вашими MQTT топиками. Используйте эту команду для добавления, удаления топиков и переключения уведомлений.\n"
        "/help - Показать это сообщение с помощью.\n"
        "\n"
        "Для взаимодействия с ботом вы можете использовать следующие команды:\n"
        "- 'Привет': Бот поздоровается с вами.\n"
        "\n"
        "Шаги для добавления топика:\n"
        "1. Используйте команду /topics.\n"
        "2. Нажмите кнопку 'Добавить топик'.\n"
        "3. Отправьте название топика, который хотите добавить.\n"
        "4. Бот подпишется на топик и будет уведомлять вас о любых сообщениях, полученных на этот топик."
        "\n"
        "Шаги для удаления топика:\n"
        "1. Используйте команду /topics.\n"
        "2. Нажмите кнопку 'Удалить топик'.\n"
        "3. Отправьте название топика, который хотите удалить.\n"
        "4. Бот отпишется от топика и удалит его из вашего списка."
        "\n"
        "Шаги для переключения уведомлений для топика:\n"
        "1. Используйте команду /topics.\n"
        "2. Нажмите кнопку 'Переключить уведомления'.\n"
        "3. Отправьте название топика, для которого хотите переключить уведомления.\n"
        "4. Бот включит или выключит уведомления для этого топика."
    )
    bot.send_message(message.chat.id, help_text)

@bot.message_handler(content_types=['text'])
def handle_text(message):
    if message.text.lower() == "привет":
        bot.send_message(message.from_user.id, "Привет, чем я могу тебе помочь?")
    else:
        bot.send_message(message.from_user.id, "Я тебя не понимаю. Напиши /help.")

# Запуск Telegram бота
bot.polling(none_stop=True, interval=0)

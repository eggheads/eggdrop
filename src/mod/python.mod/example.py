import requests
from eggdrop import bind, putmsg, findircuser
from bs4 import BeautifulSoup
from pprint import pprint

def pubGetTitle(nick, host, handle, channel, text, **kwargs):
  try:
    reqs = requests.get(text)
    soup = BeautifulSoup(reqs.text, 'html.parser')
    putmsg(channel, "The title of the webpage is: "+soup.find_all('title')[0].get_text())
  except Exception as e:
    putmsg(channel, "Error: " + str(e))

# Called from pub or msg, responds public or privately
def mypub(nick, host, handle, channel, text, **kwargs):
  putmsg(channel, f"{nick}, you said !moo")

def joinGreetUser(nick, host, handle, channel, **kwargs):
  putmsg(channel, f"Hello {nick}, welcome to {channel}")

def joinGreetOp(nick, host, handle, channel, **kwargs):
  putmsg(channel, f"Hello {nick}, welcome to {channel}, you are an operator")

badwords = ['thommey', 'eggdrop', 'python']
def checkWords(nick, host, handle, channel, text, **kwargs):
  for word in badwords:
    if word in text:
      putmsg(channel, f"I'm banning you, {nick}, for saying {word}")
      break

bind("pub", "*", "!moo", mypub)
bind("join", "*", "*", joinGreetUser)
bind("join", "*", "*", joinGreetOp)
bind("pubm", "*", "*", checkWords)
bind("pub", "*", "!title", pubGetTitle)

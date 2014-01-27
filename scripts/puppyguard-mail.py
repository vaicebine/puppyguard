#!/usr/bin/python
import smtplib, os, sys
from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.MIMEText import MIMEText
from email.Utils import formatdate
from email import Encoders

smtp_user   = sys.argv[1]
smtp_pass   = sys.argv[2]
smtp_from   = sys.argv[3]
smtp_to     = sys.argv[4]
smtp_cc     = sys.argv[5]
smtp_server = sys.argv[6]
smtp_port   = sys.argv[7]
smtp_tls    = sys.argv[8]
picture     = sys.argv[9]

SUBJECT    = "Your puppyguard have detected motion!"
TEXT       = "I have attached a picture with the potential intruder."

message = MIMEMultipart()
message['From']    = smtp_from
message['To']      = smtp_to
if smtp_cc != "0":
    message['Cc']  = smtp_cc
message['Date']    = formatdate(localtime=True)
message['Subject'] = SUBJECT
message.attach(MIMEText(TEXT))


attachment = MIMEBase('application', "octet-stream")
attachment.set_payload( open(picture,"rb").read() )
Encoders.encode_base64(attachment)
attachment.add_header('Content-Disposition', 'attachment; filename=%s' % picture)
message.attach(attachment)

server = smtplib.SMTP(smtp_server, smtp_port)
server.ehlo()

if smtp_tls == "1":
    server.starttls()
    
if smtp_user != "0":
    server.login(smtp_user, smtp_pass)
    
if smtp_cc != "0":
    server.sendmail(smtp_from, [smtp_to,smtp_cc], message.as_string())
else:
    server.sendmail(smtp_from, [smtp_to], message.as_string())

server.close()

os.remove(picture)



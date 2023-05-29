import email
import os
import sys

def extract_attachment_subparts(raw_email):
    msg = email.message_from_string(raw_email)
    attachment_subparts = []

    if msg.is_multipart():
        for part in msg.walk():
            content_type = part.get_content_type()
            disposition = part.get('Content-Disposition')

            if disposition and disposition.startswith('attachment'):
                attachment_subparts.append(part)

    return attachment_subparts

def save_attachment_bodies(attachment_subparts, directory='./attachments'):
    if not os.path.exists(directory):
        os.makedirs(directory)

    for i, attachment in enumerate(attachment_subparts, start=1):
        content_type = attachment.get_content_type()
        filename = attachment.get_filename()
        body = attachment.get_payload(decode=True)

        if not filename:
            filename = f"attachment_{i}"

        save_path = os.path.join(directory, filename)
        with open(save_path, "wb") as file:
            file.write(body)

        print(f"Attachment {i} saved to: {save_path}")

if len(sys.argv) < 2:
    print("Please provide the input file path as a command-line argument.")
    sys.exit(1)

input_file_path = sys.argv[1]

with open(input_file_path, "r") as file:
    raw_email = file.read()

attachments = extract_attachment_subparts(raw_email)
save_attachment_bodies(attachments)

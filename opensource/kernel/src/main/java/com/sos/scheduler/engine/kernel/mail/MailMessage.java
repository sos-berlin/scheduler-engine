package com.sos.scheduler.engine.kernel.mail;

import javax.activation.DataHandler;
import javax.activation.DataSource;
import javax.activation.FileTypeMap;
import javax.mail.*;
import javax.mail.Message.RecipientType;
import javax.mail.internet.*;
import java.io.*;
import java.util.*;

/** Nur für C++. */
public final class MailMessage {
    static final int current_version = 2;

    private final Properties _properties = System.getProperties();
    private final Session _session = Session.getInstance(_properties, new My_authenticator());
    private MimeMessage _msg = new MimeMessage(_session);
    private byte[] _body;
    private final List<MimeBodyPart> _attachments = new LinkedList<MimeBodyPart>();
    private final List<FileInputStream> file_input_streams = new ArrayList<FileInputStream>();      // Alle offenen Attachments, werden von close() geschlossen
    private String _smtp_user_name = "";
    private String _smtp_password = "";
    private String _encoding;
    private String _content_type;
    private boolean _built = false;

    public void close() throws Exception {
        Exception exception = null;
        for (FileInputStream o: file_input_streams) {
            try {
                o.close();
            } catch (Exception x) {
                if (exception == null) exception = x;
            }
        }
        if (exception != null) throw exception;
    }

    public void need_version(int version) throws Exception {
        if (version > current_version)
            throw new Exception("Class com.sos.scheduler.engine.kernel.mail.MailMessage (sos.mail.jar) is not up to date");
    }

    public void set(String what, byte[] value) throws MessagingException, UnsupportedEncodingException {
        if (what.equals("smtp"))
            _properties.put("mail.smtp.host", new String(value, "iso8859-1"));
        else
            //if( what.equals( "smtp.user"     ) )  _smtp_user_name = new String( value, "iso8859-1" );
            //else
            //if( what.equals( "smtp.password" ) )  _smtp_password = new String( value, "iso8859-1" );
            //else
            if (what.equals("from")) {
                InternetAddress[] addr = InternetAddress.parse(new String(value, "iso8859-1"));
                if (addr.length != 0) _msg.setFrom(addr[0]);
            } else if (what.equals("reply-to")) _msg.setReplyTo(InternetAddress.parse(new String(value, "iso8859-1")));
            else
            if (what.equals("to")) _msg.setRecipients(RecipientType.TO, InternetAddress.parse(new String(value, "iso8859-1")));
            else
            if (what.equals("cc")) _msg.setRecipients(RecipientType.CC, InternetAddress.parse(new String(value, "iso8859-1")));
            else
            if (what.equals("bcc"))
                _msg.setRecipients(RecipientType.BCC, InternetAddress.parse(new String(value, "iso8859-1")));
            else
            if (what.equals("subject")) _msg.setSubject(new String(value, "iso8859-1"));
            else
            if (what.equals("body")) {
                //_body = new MimeBodyPart();
                //_body.setText( new String(value,"iso8859-1") );
                _body = value;
            } else
            if (what.equals("content_type")) _content_type = new String(value, "iso8859-1");
            else
            if (what.equals("encoding")) _encoding = new String(value, "iso8859-1");
            else
            if (what.equals("send_rfc822")) {
                _msg = new MimeMessage(_session, new ByteArrayInputStream(value));
                send2();
            } else
            if (what.equals("debug")) _session.setDebug((new String(value, "iso8859-1")).equals("1"));
            else
                throw new RuntimeException("com.sos.scheduler.engine.kernel.mail.MailMessage.set: what");
    }

    public void set_property(String name, String value) {
        if (name.equals("mail.smtp.user")) _smtp_user_name = value;     // Keine Java-Property, Jira JS-136
        else if (name.equals("mail.smtp.password")) _smtp_password = value;      // Keine Java-Property, Jira JS-136
        else
            _properties.put(name, value);
    }

    private String string_from_addresses(Address[] addresses) {
        if (addresses == null) return "";
        String result = "";
        for (int i = 0; i < addresses.length; i++) {
            if (!result.equals("")) result = result + ", ";
            result = result + addresses[i];
        }
        return result;
    }

    public String get(String what) throws Exception {
        if (what.equals("smtp")) return (String)_properties.get("mail.smtp.host");
        else if (what.equals("from")) return string_from_addresses(_msg.getFrom());
        else if (what.equals("reply-to")) return string_from_addresses(_msg.getReplyTo());
        else if (what.equals("to")) return string_from_addresses(_msg.getRecipients(RecipientType.TO));
        else if (what.equals("cc")) return string_from_addresses(_msg.getRecipients(RecipientType.CC));
        else if (what.equals("bcc")) return string_from_addresses(_msg.getRecipients(RecipientType.BCC));
        else if (what.equals("subject")) return _msg.getSubject();
        else if (what.equals("body")) return _body == null? "" : new String(_body, "iso8859-1");
        else if (what.equals("rfc822_text")) {
            build();
            ByteArrayOutputStream os = new ByteArrayOutputStream();
            _msg.writeTo(os);
            return os.toString();
        } else
            throw new RuntimeException("com.sos.scheduler.engine.kernel.mail.MailMessage.get: what=\"" + what + "\" ist unbekannt");
    }

    public void add_header_field(String name, String value) throws MessagingException {
        _msg.setHeader(name, value);
    }

    public void add_file(String real_filename, String new_filename, String content_type, String encoding) throws Exception {
        if (new_filename == null || new_filename.length() == 0) new_filename = real_filename;
        if (content_type == null || content_type.length() == 0)
            content_type = FileTypeMap.getDefaultFileTypeMap().getContentType(new_filename);

        MimeBodyPart attachment = new MimeBodyPart();

        DataSource data_source = new File_data_source(new File(real_filename), new File(new_filename), content_type);
        DataHandler data_handler = new DataHandler(data_source);

        attachment.setDataHandler(data_handler);
        attachment.setFileName(data_handler.getName());

        _attachments.add(attachment);
    }

    public void add_attachment(byte[] data, String filename, String content_type, String encoding) throws MessagingException {
        if (content_type.length() == 0) content_type = FileTypeMap.getDefaultFileTypeMap().getContentType(filename);

        MimeBodyPart attachment = new MimeBodyPart();

        DataSource data_source = new Byte_array_data_source(data, new File(filename), content_type);
        DataHandler data_handler = new DataHandler(data_source);

        attachment.setDataHandler(data_handler);
        attachment.setFileName(data_handler.getName());

        _attachments.add(attachment);
    }

    public void send() throws Exception {
        build();
        send2();
    }

    public void build() throws Exception {
        if (_built) return;

        _msg.setSentDate(new Date());     // Damit rfc822_text das Datum liefert. Jira JS-81

        if (_content_type == null || _content_type.equals("")) _content_type = "text/plain";

        if (_attachments.size() == 0) {
            set_body_in(_msg);
        } else {
            MimeMultipart multipart = new MimeMultipart();

            MimeBodyPart b = new MimeBodyPart();
            set_body_in(b);
            multipart.addBodyPart(b);

            for (MimeBodyPart a: _attachments) multipart.addBodyPart(a);

            _msg.setContent(multipart);
        }

        _built = true;
    }

    private void set_body_in(MimePart body_part) throws Exception {
        body_part.setContent(new String(_body, "iso8859-1"), _content_type);
    }

    protected void send2() throws MessagingException {
        if (_smtp_user_name.length() > 0) _properties.put("mail.smtp.auth", "true");
        Transport.send(_msg);
    }

    abstract static class My_data_source implements DataSource {
        private final String name;
        private final String content_type;

        protected My_data_source(File new_filename, String content_type) {
            this.name = new_filename.getName();
            this.content_type = content_type;
        }

        public String getContentType() {
            return content_type;
        }

        public String getName() {
            return name;
        }

        public OutputStream getOutputStream() {
            throw new UnsupportedOperationException(getClass().getName() + " has no OutputStream");
        }
    }

    static final class Byte_array_data_source extends My_data_source {
        private final byte[] byte_array;

        Byte_array_data_source(byte[] byte_array, File new_filename, String content_type) {
            super(new_filename, content_type);
            this.byte_array = byte_array;
        }

        public InputStream getInputStream() {
            return new ByteArrayInputStream(byte_array);
        }
    }


    final class File_data_source extends My_data_source {
        // M�glicherweise kann FileDataSource verwendet werden.
        // Aber schlie�t die Klasse die Datei? Es gibt keinen close()
        private final File file;

        File_data_source(File file, File new_filename, String content_type) {
            super(new_filename, content_type);
            this.file = file;
        }

        public InputStream getInputStream() throws IOException {
            FileInputStream f = new FileInputStream(file);
            file_input_streams.add(f);    // wird von MailMessage.close() geschlossen
            return f;
        }
    }

    public class My_authenticator extends Authenticator {
        public PasswordAuthentication getPasswordAuthentication() {
            //System.err.print( "getPasswordAuthentication " + _smtp_user_name + ", " + _smtp_password + "\n" );
            return new PasswordAuthentication(_smtp_user_name, _smtp_password);
        }
    }
}

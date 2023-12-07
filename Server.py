from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import os

# Estado inicial de los sensores y actuadores
ledState = False
smokeDetected = False
motionDetected = False
buzzerActive = False

class MyHTTPRequestHandler(BaseHTTPRequestHandler):

    def _set_response(self, content_type="text/plain", status_code=200):
        self.send_response(status_code)
        self.send_header("Content-type", content_type)
        self.end_headers()

    def do_GET(self):
        global ledState, smokeDetected, motionDetected, buzzerActive
        print(self.path)
        if self.path == "/":
            try:
                # Get the absolute path to the HTML file
                self._set_response(content_type="text/html")
                html_file_path = os.path.abspath("index.html")
                with open(html_file_path, "r", encoding="utf-8") as file_to_open:
                    # Write the HTML content to the response
                    self.wfile.write(file_to_open.read().encode())
            except Exception as e:
                print(f"Error: {e}")
                self.wfile.write(f"Error: {e}".encode())

        elif self.path == "/led":
            self._set_response("application/json")
            self.wfile.write(json.dumps({"status": ledState}).encode())
        
        elif self.path == "/smoke":
            self._set_response("application/json")
            self.wfile.write(json.dumps({"smokeDetected": smokeDetected}).encode())
        
        elif self.path == "/motion":
            self._set_response("application/json")
            self.wfile.write(json.dumps({"motionDetected": motionDetected}).encode())
        
        else:
            self._set_response("application/json", 500)
            self.wfile.write(json.dumps({"error": str(e)}).encode())


    def do_POST(self):
        content_length = int(self.headers["Content-Length"])
        post_data = self.rfile.read(content_length).decode()

        global ledState, smokeDetected, motionDetected, buzzerActive

        try:
            body_json = json.loads(post_data)
        except json.JSONDecodeError:
            self._set_response("application/json", 400)
            self.wfile.write(json.dumps({"message": "Invalid JSON"}).encode())
            return

        if self.path == "/smoke":
            smokeDetected = body_json.get("smokeDetected", False)

        if self.path == "/motion":
            motionDetected = body_json.get("motionDetected", False)

        if self.path == "/buzzer/on":
            buzzerActive = True
            self._set_response("application/json")
            self.wfile.write(json.dumps({"message": "Buzzer turned on"}).encode())

        if self.path == "/buzzer/off":
            buzzerActive = False
            self._set_response("application/json")
            self.wfile.write(json.dumps({"message": "Buzzer turned off"}).encode())

        self._set_response("application/json")
        self.wfile.write(json.dumps({"message": "Data updated successfully"}).encode())


def run_server(server_class=HTTPServer, handler_class=MyHTTPRequestHandler, port=7800):
    server_address = ("", port)
    httpd = server_class(server_address, handler_class)
    print(f"Starting server on port {port}...")
    httpd.serve_forever()

if __name__ == "__main__":
    run_server()

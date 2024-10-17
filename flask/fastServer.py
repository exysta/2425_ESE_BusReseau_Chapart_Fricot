from fastapi import FastAPI, HTTPException, Request
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from typing import Optional, Dict

app = FastAPI()

users = [
    {"id": 1, "name": "lulu"},
    {"id": 2, "name": "chacha"},
]

class WelcomeMessage(BaseModel):
    welcome_message: Optional[str] = None

class WordData(BaseModel):
    word: str

class LetterData(BaseModel):
    letter: str

welcome = "Welcome to 3ESE API! of lulu and chacha"

@app.get("/")
def hello_world():
    return "Hello, World it's me again!\n"

@app.api_route("/api/welcome/", methods=["GET", "POST", "RETRIEVE", "DELETE"])
async def api_welcome(request: Request, data: Optional[WelcomeMessage] = None):
    global welcome
    
    if request.method == 'POST':
        if data and data.welcome_message:
            welcome = data.welcome_message
            return {"message": "Welcome message updated."}
    
    elif request.method == 'RETRIEVE':
        return welcome
    
    elif request.method == 'DELETE':
        welcome = ""
        return {"message": "Welcome message deleted."}
    
    elif request.method == 'GET':
        return welcome

@app.api_route("/api/welcome/{index}", methods=["GET", "POST", "PUT", "PATCH", "DELETE"])
async def api_welcome_index(index: int, request: Request, word_data: Optional[WordData] = None, letter_data: Optional[LetterData] = None):
    global welcome
    
    if index < 0 or index >= len(welcome):
        raise HTTPException(status_code=404, detail="Index out of range")
    
    if request.method == 'GET':
        return {"index de la lettre": index, "lettre": welcome[index]}
    
    elif request.method == 'PUT':
        if word_data and word_data.word:
            welcome = welcome[:index] + word_data.word + welcome[index:]
            return {"mot": word_data.word, "a été inséré à l'index": index}
    
    elif request.method == 'PATCH':
        if letter_data and letter_data.letter:
            welcome = welcome[:index] + letter_data.letter + welcome[index:]
            return {"lettre à l'index": index, "à été remplacé par": welcome[index]}
    
    elif request.method == 'DELETE':
        welcome = welcome[:index] + welcome[index + 1:]
        return {"message": f"lettre à l'index {index} a été supprimé"}

def find_user(user_id: int):
    return next((user for user in users if user["id"] == user_id), None)

@app.exception_handler(404)
async def page_not_found(request: Request, exc: HTTPException):
    return JSONResponse(status_code=404, content={"message": "Page not found"})

@app.api_route("/api/request/", methods=["GET", "POST"])
@app.api_route("/api/request/{path:path}", methods=["GET", "POST"])
async def api_request(request: Request, path: Optional[str] = None):
    resp = {
        "method": request.method,
        "url": str(request.url),
        "path": path,
        "args": dict(request.query_params),
        "headers": dict(request.headers),
    }
    if request.method == 'POST':
        resp["POST"] = {
            "data": await request.json(),
        }
    return resp

if __name__ == '__main__':
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=5000)
# /// [Functional test]
async def test_ping(service_client):
    response = await service_client.get('/hello')
    assert response.status == 200
    assert response.content == b'Hello world!\n'
    assert 'X-RequestId' not in response.headers.keys(), 'Unexpected header'
    # /// [Functional test]


async def test_wrong_method(service_client):
    response = await service_client.request('KEK', '/hello')
    assert response.status == 400
    assert response.content == b'bad request'
    assert 'X-YaRequestId' not in response.headers.keys(), 'Unexpected header'

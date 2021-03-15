# Order of the Overflow DEFCON CTF Quals Template

This serves as the template for OOO's quals challenges -- jeopardy style. A/D and KotH challenges have a similar, but slightly different template.

## Writing the service

Your service is be built from `service/Dockerfile` dockerfile.
The contents of that dockerfile, and the security of the resulting container, are 100% up to the author. It's deployed as-is in kubernetes, the main tricky things being:

1. The number of containers to spawn
2. CPU and memory resources assigned to each container.

There are no specific expectations except for:

1. Your service should `EXPOSE` one (and only one) port.
   Port deduplication across services can be handled by the quals infrastructure, so keeping the example port is fine.
   This template wraps a commandline service in xinetd.
2. Your service should automatically run when `docker run -d your-service` is performed.
   The template does this by launching xinetd.
3. Egress is not allowed.

## Specifying public files.

Public files are specified in the yaml, relative to the git repo root.

These are the files exposed to players, currently with individual links to the base filename only.

For internal reference, we also bundle them into a `public_bundle.tar.gz`.

## Writing the exploit and test scripts

Your test scripts will be launched through a docker image built from the `interactions/Dockerfile` dockerfile.
This dockerfile should set up the necessary environment to run interactions.
Every script specified in your yaml's `interactions` list will be run.
If a filename starts with the word `exploit`, then it is assumed to be an exploit that prints out the flag (anywhere in stdout), and its output is checked for the flag.

The interaction image is instantiated as a persistent container in which the tests are run.
Your test scripts should *not* be its `entrypoint` or `cmd` directives.
An approximation of the test procedure is, roughly:

```
INTERACTION_CONTAINER=$(docker run --rm -d $INTERACTION_IMAGE)
docker exec $INTERACTION_CONTAINER /exploit1.py $SERVICE_IP $SERVICE_PORT) | grep OOO{blahblah}
docker exec $INTERACTION_CONTAINER /exploit2.sh $SERVICE_IP $SERVICE_PORT) | grep OOO{blahblah}
docker exec $INTERACTION_CONTAINER /check1.py $SERVICE_IP $SERVICE_PORT)
docker exec $INTERACTION_CONTAINER /check2.sh $SERVICE_IP $SERVICE_PORT)
docker kill $INTERACTION_CONTAINER
```

The tester will stress-test your service by launching multiple concurrent interactions. This is controlled by a parameter in the info.yml.

It'll also stress-test it by launching multiple connections that terminate immediately (aka short-reads), and making sure your service cleanly exits in this scenario!

## Resource planning

Auto-scaling is tricky and we (OOO) don't currently do it.

Instead, we do some back-of-the-envelope calculations for the number of `instances` (xinetd) and/or `concurrent_connections` (tester), times the number of expected container replicas.

In addition, the occupied resources should fit in the limits and "make sense" for the available kubernetes nodes. Factor in the timeout (there MUST be one!) and the resource limits that are imposed on each connection. See the `wrapper` and the example xinetd `service.conf`.

We aggressively healthcheck (banner grep) services during the game + we have alerts for resource consumption, so if we get a higher-than-anticipated load we scale manually as-needed.

Finally, there are services for which we suspect players will be tempted to overly stress the infrastructure -- typically by attempting impossible bruteforces instead of correctly solving the challenge offline. OOO's quals infrastructure can transparently request Proof-Of-Work in these cases. We have a [helper script for players](https://oooverflow.io/pow.py) but many players hate this measure as it applies to good and bad players alike. Sometimes the mere threat of a PoW is enough to make players behave. Sometimes... it's not.

## Building

The tester needs `PyYAML`, and `coloredlogs` is recommended too. Docker is needed, obviously, but the tester just uses its cli.

You can build and test your service with:

```
# do everything
./tester

#
# or, piecemeal:
#

# build the service and public bundle:
./tester build

# launch and test the service (interaction tests, stress tests, shortreads)
./tester test

# just launch the service (will print network endpoint info)
./tester launch
```

Once ready, you can:

```
# push to the infra (the first time we also manually create the service in k8s)
./tester deploy

# live-test
./tester test_deployed
```

The following artifacts result from the build process:

1. a `NAME_PREFIX:your_service` image is created, containing your service
1. a `NAME_PREFIX:your_service-interactions` image is created, with your interaction scripts
1. `public_bundle.tar.gz` has the public files, extracted from the service image.

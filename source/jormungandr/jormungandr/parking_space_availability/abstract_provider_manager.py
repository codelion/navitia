from __future__ import absolute_import, print_function, unicode_literals, division
from abc import abstractmethod, ABCMeta
import six
import logging
import sys

def get_from_to_pois_of_journeys(journeys):
    from_to_places = (
        s.get(from_to, {}) for j in journeys for s in j.get('sections', []) for from_to in ('from', 'to')
    )
    return (place['poi'] for place in from_to_places if 'poi' in place)

class AbstractProviderManager(six.with_metaclass(ABCMeta, object)):
    def __init__(self):
        self.log = logging.getLogger(__name__)

    @abstractmethod
    def _handle_poi(self, item):
        pass

    @abstractmethod
    def _get_providers(self):
        pass

    def handle(self, response, attribute):
        if attribute == 'journeys':
            return self.handle_journeys(response[attribute])
        elif attribute in ('places', 'places_nearby', 'pois'):
            return self.handle_places(response[attribute])
        return None

    def handle_places(self, places):
        providers = set()
        for place in places or []:
            provider = None
            if 'poi_type' in place:
                provider = self._handle_poi(place)
            elif 'embedded_type' in place and place['embedded_type'] == 'poi':
                provider = self._handle_poi(place['poi'])
            if provider:
                providers.add(provider)
        return providers

    def handle_journeys(self, journeys):
        providers = set()
        for poi in get_from_to_pois_of_journeys(journeys):
            provider = self._handle_poi(poi)
            if provider:
                providers.add(provider)
        return providers

    def _find_provider(self, poi):
        for provider in self._get_providers():
            if provider.handle_poi(poi):
                return provider
        return None

    def _init_class(self, cls, arguments):
        try:
            module_path, name = cls.rsplit('.', 1)
            if module_path not in sys.modules:
                raise ImportError("Invalid module path")
            module = sys.modules[module_path]
            attr = getattr(module, name)
            return attr(**arguments)
        except ImportError:
            pass
            self.log.warning('impossible to build, cannot find class: {}'.format(cls))